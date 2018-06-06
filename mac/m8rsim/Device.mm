//
//  Device.mm
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Device.h"

#import "FastSocket.h"
#import "Simulator.h"
#import "Shell.h"
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>
#import <ctime>
#import <thread>
#import <chrono>
#import <MGSFragaria/MGSFragaria.h>

#define Prompt '>'

class MyGPIOInterface;

@interface Device ()
{
    NSNetServiceBrowser* _netServiceBrowser;
    NSMutableArray<NSDictionary*>* _devices;
    NSDictionary* _currentDevice;
    FileList _fileList;
    
    Simulator* _simulator;

    FastSocket* _logSocket;
    FastSocket* _shellSocket;
    
    dispatch_queue_t _shellQueue;
    dispatch_queue_t _logQueue;

    BOOL _isBuild;
    
    std::unique_ptr<MyGPIOInterface> _gpio;
}

- (void)outputMessage:(NSString*)msg, ...;
- (void)updateGPIOState:(uint32_t) state withMode:(uint32_t) mode;
- (void)updateMemoryInfo;

@end

class MyGPIOInterface : public m8r::GPIOInterface {
public:
    MyGPIOInterface(Device* device) : _device(device) { }
    virtual ~MyGPIOInterface() { }

    virtual bool setPinMode(uint8_t pin, PinMode mode) override
    {
        if (!GPIOInterface::setPinMode(pin, mode)) {
            return false;
        }
        _pinio = (_pinio & ~(1 << pin)) | ((mode == PinMode::Output) ? (1 << pin) : 0);

        [_device updateGPIOState:_pinstate withMode:_pinio];
        return true;
    }
    
    virtual bool digitalRead(uint8_t pin) const override
    {
        return _pinstate & (1 << pin);
    }
    
    virtual void digitalWrite(uint8_t pin, bool level) override
    {
        if (pin > 16) {
            return;
        }
        _pinstate = (_pinstate & ~(1 << pin)) | (level ? (1 << pin) : 0);

        [_device updateGPIOState:_pinstate withMode:_pinio];
    }
    
    virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) override { }
    
private:
    // 0 = input, 1 = output
    uint32_t _pinio = 0;
    uint32_t _pinstate = 0;
    
    Device* _device;
};

@implementation Device

- (instancetype)init
{
    self = [super init];
    if (self) {
        _devices = [[NSMutableArray alloc] init];
        _fileList = [[NSMutableArray alloc] init];
        
        _gpio.reset(new MyGPIOInterface(self));

        _simulator = new Simulator(_gpio.get());
        _shellQueue = dispatch_queue_create("ShellQueue", DISPATCH_QUEUE_SERIAL);
        _logQueue = dispatch_queue_create("LogQueue", DISPATCH_QUEUE_SERIAL);
    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
        
        [self setDevice:@""];
    }
    return self;
}

- (void)dealloc
{
     delete _simulator;
}

- (void)outputMessage:(NSString*)msg, ...
{
    va_list args;
    va_start(args, msg);
    NSString* string = [[NSString alloc] initWithFormat:msg arguments:args];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate outputMessage:string toBuild:_isBuild];
    });
}

- (void)updateGPIOState:(uint32_t) state withMode:(uint32_t) mode
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate updateGPIOState:state withMode:mode];
    });
}

- (void)flushToPrompt
{
    while(1) {
        char c;
        long count = [_shellSocket receiveBytes:&c count:1];
        if (count != 1 || c == Prompt) {
            break;
        }
    }
}

- (NSString*) receiveToTerminator:(char) terminator
{
    NSMutableString* s = [NSMutableString string];
    char c[2];
    c[1] = '\0';
    while(1) {
        long count = [_shellSocket receiveBytes:c count:1];
        if (count != 1 || c[0] == terminator) {
            break;
        }
        [s appendString:[NSString stringWithUTF8String:c]];
    }
#ifdef MONITOR_TRAFFIC
    NSLog(@"[Device] <<<< receiveToTerminator(%c):'%@'\n", terminator, s);
#endif
    return s;
}

- (void)sendCommand:(NSString*)command andString:(NSString*) string
{
    NSData* data = [command dataUsingEncoding:NSUTF8StringEncoding];
    long count = [_shellSocket sendBytes:data.bytes count:data.length];
    assert(count == data.length);
    (void) count;
#ifdef MONITOR_TRAFFIC
    NSLog(@"[Device] >>>> sendCommand:'%@'\n", command);
#endif

    if (string) {
        NSData* data = [string dataUsingEncoding:NSUTF8StringEncoding];
        long count = [_shellSocket sendBytes:data.bytes count:data.length];
        assert(count == data.length);
        (void) count;
#ifdef MONITOR_TRAFFIC
        NSLog(@"[Device] >>>> andString:'%@'\n", string);
#endif
    }
}

- (NSString*)sendCommand:(NSString*)command withTerminator:(char)terminator
{
    [self sendCommand:command andString:nil];
    NSString* s = [self receiveToTerminator:terminator];
    return s;
}

- (NSArray*) fileListForDevice
{
    NSMutableArray* array = [[NSMutableArray alloc]init];
    NSString* fileString = [self sendCommand:@"ls\r\n" withTerminator:Prompt];
    if (fileString && fileString.length > 0 && [fileString characterAtIndex:0] == ' ') {
        fileString = [fileString substringFromIndex:1];
    }

    NSArray* lines = [fileString componentsSeparatedByString:@"\n"];
    for (NSString* line in lines) {
        NSArray* elements = [line componentsSeparatedByString:@":"];
        if (elements.count != 3 || ![elements[0] isEqualToString:@"file"]) {
            continue;
        }
    
        NSString* name = elements[1];
        NSString* size = elements[2];
    
        // Ignore . files
        if ([name length] == 0 || [name characterAtIndex:0] == '.') {
            continue;
        }
    
        [array addObject:@{ @"name" : name, @"size" : size }];
    }
    
    [self updateMemoryInfo];
    return array;
}

- (void)reloadFilesWithBlock:(void (^)(FileList))handler
{
    [_fileList removeAllObjects];
    NSArray* fileList = [self fileListForDevice];
    for (NSDictionary* fileEntry in fileList) {
        [_fileList addObject:fileEntry];
    }
    
    [_fileList sortUsingComparator:^NSComparisonResult(NSDictionary* a, NSDictionary* b) {
        return [a[@"name"] compare:b[@"name"]];
    }];

    dispatch_async(dispatch_get_main_queue(), ^{
        handler(_fileList);
    });
}

- (void)loadFilesWithURL:(NSURL*)url
{
    NSError* error;
    if (!_simulator->setFiles(url, &error)) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"OK"];
            [alert setMessageText:@"Invalid project file"];
            [alert setInformativeText:@"File is not a valid m8r project or could not be opened"];
            [alert setAlertStyle:NSWarningAlertStyle];
            [alert runModal];
        });
    }
    
    [_delegate reloadFiles];
}

- (void)reloadDevices
{
}

- (NSData*)contentsOfFile:(NSString*)name
{
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command withTerminator:'\04'];
    
    NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    [self updateMemoryInfo];
    return data;
}

- (void)selectFile:(NSInteger)index
{
    NSString* name = _fileList[index][@"name"];
    
    dispatch_async(_shellQueue, ^() {        
        NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
        NSString* fileContents = [self sendCommand:command withTerminator:'\04'];
        [self flushToPrompt];
        NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.delegate setContents:data withName:name];
        });
        [self updateMemoryInfo];
    });
}

- (BOOL)saveFile:(NSString*)name withURLBase:(NSURL*)urlBase
{
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command withTerminator:'\04'];
    NSURL* url = [NSURL URLWithString:name relativeToURL:urlBase];
    NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    [self updateMemoryInfo];
    return [data writeToURL:url atomically:YES];
}

- (BOOL) isFile:(NSString*)name inFileList:(NSArray*)fileList
{
    for (NSDictionary* entry in fileList) {
        if ([entry[@"name"] isEqualToString:name]) {
            return YES;
        }
    }
    return NO;
}

- (void)updateFile:(NSString*)name withContents:(NSData*)contents
{
    NSFileWrapper* files = _simulator->getFiles();
    if (files) {
        [files removeFileWrapper:files.fileWrappers[name]];
        [files addRegularFileWithContents:contents preferredFilename:name];
    }
}

- (void)addFile:(NSString*)name withContents:(NSData*)contents
{
    NSString* contentString = [contents base64EncodedStringWithOptions:
                                                            NSDataBase64Encoding64CharacterLineLength |
                                                            NSDataBase64EncodingEndLineWithCarriageReturn | 
                                                            NSDataBase64EncodingEndLineWithLineFeed];
    NSString* command = [NSString stringWithFormat:@"put %@\r\n", name];
    contentString = [NSString stringWithFormat:@"%@\r\n\04", contentString];

    [self sendCommand:command andString:contentString];
    [self flushToPrompt];
    [self updateMemoryInfo];
}

- (void)mirrorFiles
{
    if (!_currentDevice) {
        return;
    }
    
    NSArray* deviceFileList = [self fileListForDevice];
    NSArray* localFileList = _simulator->listFiles();
    
    // Delete files in device but not in local
    for (NSDictionary* entry in deviceFileList) {
        NSString* name = entry[@"name"];
        if (![self isFile:name inFileList:localFileList]) {
            [self removeFile:name];
        }
    }
    
    // Add files that are new or that have changed
    for (NSDictionary* entry in localFileList) {
        NSString* name = entry[@"name"];
        NSData* contents = _simulator->getFileData(name);
        if ([self isFile:name inFileList:deviceFileList]) {
            // If contents are different, replace
            NSData* deviceContents = [self contentsOfFile:name];
            if (deviceContents.length == contents.length) {
                if (memcmp(deviceContents.bytes, contents.bytes, contents.length) == 0) {
                    continue;
                }
            }
        }

        [self addFile:name withContents:contents];
    }
}

- (void)addFile:(NSFileWrapper*)fileWrapper
{
    [self addFile:fileWrapper.preferredFilename withContents:fileWrapper.regularFileContents];
    dispatch_async(dispatch_get_main_queue(), ^{
        [_delegate markDirty];
    });
}

- (void)removeFile:(NSString*)name
{
    dispatch_async(_shellQueue, ^() {        
        NSString* command = [NSString stringWithFormat:@"rm %@\r\n", name];
        [self sendCommand:command withTerminator:Prompt];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate markDirty];
        });
        [self updateMemoryInfo];
    });
}

- (void)renameFileFrom:(NSString*)oldName to:(NSString*)newName
{
    NSString* __block command = [NSString stringWithFormat:@"mv %@ %@\r\n", oldName, newName];
    dispatch_async(_shellQueue, ^() {        
        [self sendCommand:command withTerminator:Prompt];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate markDirty];
        });
        [self updateMemoryInfo];
    });
}

- (void)updateMemoryInfo
{
    NSString* __block command = @"heap\r\n";
    dispatch_async(_shellQueue, ^() {        
        NSString* sizeString = [self sendCommand:command withTerminator:Prompt];
        NSArray* elements = [sizeString componentsSeparatedByString:@":"];
        if (elements.count != 5 || ![elements[0] isEqualToString:@"heap"]) {
            return;
        }
        NSUInteger size = [[elements objectAtIndex:1] intValue];
        NSUInteger numObj = [[elements objectAtIndex:2] intValue];
        NSUInteger numStr = [[elements objectAtIndex:3] intValue];
        NSUInteger numOth = [[elements objectAtIndex:4] intValue];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate setFreeMemory:size numObj:numObj numStr:numStr numOther: numOth];
        });
    });
}

- (NSString*)trimTrailingDot:(NSString*)s
{
    if ([s characterAtIndex:s.length - 1] == '.') {
        return [s substringToIndex:s.length - 1];
    }
    return s;
}

- (NSNetService*)serviceFromDevice:(NSDictionary*)device
{
    return (NSNetService*) (device[@"service"]);
}

- (NSDictionary*) findService:(NSString*)hostname
{
    for (NSDictionary* service in _devices) {
        if ([hostname isEqualToString:[self trimTrailingDot:[self serviceFromDevice:service].hostName]]) {
            return service;
        }
    }
    return nil;
}

- (BOOL)setDevice:(NSString*)device
{
    [_delegate reloadFiles];

    if (_logSocket) {
        _logSocket = nil;
        dispatch_sync(_logQueue, ^{ });
    }

    if (_shellSocket) {
        _shellSocket = nil;
        dispatch_sync(_logQueue, ^{ });
    }

    _currentDevice = [self findService:device];
    if (!_currentDevice && !_simulator) {
        [self outputMessage:@"**** No simulator\n"];
        return NO;
    }

    NSString* ipString = @"127.0.0.1";
    NSUInteger shellPort = _simulator->localPort();
    NSUInteger logPort = _simulator->localPort() + 1;
    
    if (_currentDevice) {
        NSNetService* service = _currentDevice[@"service"];
        if (service.addresses.count == 0) {
            return NO;
        }

        NSData* address = [service.addresses objectAtIndex:0];
        struct sockaddr_in * socketAddress = (struct sockaddr_in *) address.bytes;
        ipString = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
        shellPort = service.port;
        logPort = service.port + 1;
    }
    
    NSString* portString = [NSNumber numberWithInteger:logPort].stringValue;

    _logSocket = [[FastSocket alloc]initWithHost:ipString andPort:portString];
    [_logSocket setTimeout:7200];

    dispatch_async(_logQueue, ^{
        if ([_logSocket connect]) {
            while (1) {
                char buffer[100];
                long count = [_logSocket receiveBytes:buffer limit:99];
                if (count == 0) {
                    break;
                }
                buffer[count] = '\0';
                [self outputMessage:[NSString stringWithUTF8String:buffer]];
            }
        } else {
            [self outputMessage:@"*** Could not open log socket: %@\n", _logSocket.lastError.localizedDescription];
        }
    });
    
    portString = [NSNumber numberWithInteger:shellPort].stringValue;
    _shellSocket = [[FastSocket alloc] initWithHost:ipString andPort:portString];
    [_shellSocket setTimeout:5];

    dispatch_async(_shellQueue, ^{
        if (![_shellSocket connect]) {
            [self outputMessage:@"*** Could not open shell socket: %@\n", _shellSocket.lastError.localizedDescription];
        }
        [self flushToPrompt];
    });

    return YES;
}

- (void)reloadDeviceList
{
}

- (void)renameDevice:(NSString*)name
{
    dispatch_async(_shellQueue, ^() {
        NSString* command = [NSString stringWithFormat:@"dev %@\r\n", name];
        
        NSString* s = [self sendCommand:command withTerminator:Prompt];
        NSLog(@"renameDevice returned '%@'", s);
    });
    [self updateMemoryInfo];
}

- (BOOL)canRun
{
    return true;
}

- (BOOL)canBuild
{
    return _currentDevice == nil;
}

- (BOOL)canStop
{
    return true;
}

- (BOOL)canUpload
{
    return _currentDevice != nil;
}

- (BOOL)canSimulate
{
    return true;
}

- (BOOL)canSaveBinary
{
    return _currentDevice == nil;
}

- (NSArray*)buildFile:(NSString*) name withDebug:(BOOL)debug
{
    _isBuild = YES;
    if (debug) {
        NSString* command = [NSString stringWithFormat:@"debug\r\n"];
        [self sendCommand:command withTerminator:Prompt];
    }
    
    NSString* command = [NSString stringWithFormat:@"build %@\r\n", name];
    NSString* errors = [self sendCommand:command withTerminator:Prompt];
    [self updateMemoryInfo];
    
    if (!errors || ![errors length]) {
        _simulator->printCode();
        return nil;
    }
    
    NSMutableArray* errorArray = [[NSMutableArray alloc] init];

    NSArray* lines = [errors componentsSeparatedByString:@"\n"];

    for (NSString* line in lines) {
        NSArray* elements = [line componentsSeparatedByString:@"::"];
        if (elements.count != 4) {
            continue;
        }
        
        SMLSyntaxError *syntaxError = [SMLSyntaxError new];
        syntaxError.description = [elements objectAtIndex:0];
        syntaxError.line = [[elements objectAtIndex:1] intValue];
        syntaxError.character = [[elements objectAtIndex:2] intValue];
        syntaxError.length = [[elements objectAtIndex:3] intValue];
        [errorArray addObject:syntaxError];
    }
    return errorArray;
}

- (void)runFile:(NSString*) name withDebug:(BOOL)debug
{
    _isBuild = NO;
    dispatch_async(_shellQueue, ^() {        
        if (debug) {
            NSString* command = [NSString stringWithFormat:@"debug\r\n"];
            [self sendCommand:command withTerminator:Prompt];
        }
        
        NSString* command = [NSString stringWithFormat:@"run %@\r\n", name];
        [self sendCommand:command withTerminator:Prompt];
        dispatch_async(dispatch_get_main_queue(), ^{
        });
        [self updateMemoryInfo];
    });
}

- (void)pause
{
    // FIXME: Implement
}

- (void)stop
{
    dispatch_async(_shellQueue, ^() {        
        NSString* command = [NSString stringWithFormat:@"stop\r\n"];
        [self sendCommand:command withTerminator:Prompt];
        dispatch_async(dispatch_get_main_queue(), ^{
        });
        [self updateMemoryInfo];
    });
}

- (void)simulate
{
    // FIXME: Implement
}

- (void)saveFilesToURL:(NSURL*)url
{
    NSFileWrapper* files = _simulator->getFiles();
    NSFileWrapper *contentsFileWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Files" : files }];
    NSFileWrapper* package = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Contents" : contentsFileWrapper }];

    NSError* error;
    if (![package writeToURL:url options:0 originalContentsURL:NULL error:&error]) {
        NSLog(@"***** Error - saveFilesToURL:%@", error);
    }
}

- (BOOL)saveChangedFiles
{
    if (_currentDevice != nil) {
        return YES;
    }
    
    return NO;
}

- (BOOL)isDeviceFile
{
    return _currentDevice != nil;
}

- (void)clearContents
{
    dispatch_async(_shellQueue, ^() {        
        NSString* command = [NSString stringWithFormat:@"clear\r\n"];
        [self sendCommand:command withTerminator:Prompt];
        [self updateMemoryInfo];
    });
}

// NSNetServiceBrowser delegate
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser
           didFindService:(NSNetService *)netService
               moreComing:(BOOL)moreServicesComing
{
    [_devices addObject:@{
        @"service" : netService
    }];
        
    [netService setDelegate:(id) self];
    NSLog(@"*** Found service: %@\n", netService);
    [netService resolveWithTimeout:10];
}

-(void)netServiceBrowser:(NSNetServiceBrowser *)browser didNotSearch:(NSDictionary<NSString *,NSNumber *> *)errorDict {
    NSLog(@"not search ");
}

- (void)netServiceDidResolveAddress:(NSNetService *)sender
{
    NSString* hostName = [self trimTrailingDot:sender.hostName];
    [_delegate addDevice:hostName];
}

- (void)netService:(NSNetService *)sender 
     didNotResolve:(NSDictionary<NSString *,NSNumber *> *)errorDict
{
    NSLog(@"********* Did not resolve: %@\n", errorDict);
}

@end
