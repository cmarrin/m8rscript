//
//  Device.mm
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Device.h"

#import "FastSocket.h"
#import "MacFS.h"
#import "Simulator.h"
#import "MacUDP.h"
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>

class DeviceSystemInterface;

@interface Device ()
{
    NSNetServiceBrowser* _netServiceBrowser;
    NSMutableArray<NSDictionary*>* _devices;
    NSDictionary* _currentDevice;
    FileList _fileList;
    NSFileWrapper* _files;

    DeviceSystemInterface* _system;
    Simulator* _simulator;
    FastSocket* _logSocket;
    
    dispatch_queue_t _serialQueue;
    dispatch_queue_t _logQueue;
}

- (void)outputMessage:(NSString*)msg;
- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;
@end

class DeviceSystemInterface : public m8r::SystemInterface
{
public:
    DeviceSystemInterface(Device* device) : _device(device) { }
    
    virtual void vprintf(const char* s, va_list args) const override
    {
        NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
        [_device outputMessage:string];
    }
    
    virtual void updateGPIOState(uint16_t mode, uint16_t state) override
    {
        [_device updateGPIOState:state withMode:mode];
    }
    
    virtual int read() const override { return -1; }

private:
    Device* _device;
};

@implementation Device

- (instancetype)init
{
    self = [super init];
    if (self) {
        _devices = [[NSMutableArray alloc] init];
        _fileList = [[NSMutableArray alloc] init];
        [self setFiles:[[NSFileWrapper alloc]initDirectoryWithFileWrappers:@{ }]];

        _system = new DeviceSystemInterface(self);
        _simulator = new Simulator(_system);
        
        _serialQueue = dispatch_queue_create("DeviceQueue", DISPATCH_QUEUE_SERIAL);

    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
        
        (void) m8r::IPAddr::myIPAddr();
     }
    return self;
}

- (void)dealloc
{
    if (_simulator->isRunning()) {
        _simulator->stop();
        dispatch_sync(_serialQueue, ^{ });
    }
    delete _simulator;
    delete _system;
}

- (void)outputMessage:(NSString*)msg
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate outputMessage:msg toBuild:_simulator->isBuild()];
    });
}

- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode
{
    [self.delegate updateGPIOState:state withMode:mode];
}

- (void)flushToPrompt:(FastSocket*) socket
{
    while(1) {
        char c;
        long count = socket ? [socket receiveBytes:&c count:1] : _simulator->receiveFromShell(&c, 1);
        if (count != 1 || c == '>') {
            break;
        }
    }
}

- (NSString*) receiveFrom:(FastSocket*) socket toTerminator:(char) terminator
{
    NSMutableString* s = [NSMutableString string];
    char c[2];
    c[1] = '\0';
    while(1) {
        long count = socket ? [socket receiveBytes:c count:1] : _simulator->receiveFromShell(&c, 1);
        if (count != 1 || c[0] == terminator) {
            break;
        }
        [s appendString:[NSString stringWithUTF8String:c]];
    }
    return s;
}

- (FastSocket*)sendCommand:(NSString*)command fromService:(NSNetService*)service
{
    FastSocket* socket = nullptr;
    
    if (!service) {
        _simulator->initShell();
    } else {
        if (service.addresses.count == 0) {
            return nil;
        }
        NSData* address = [service.addresses objectAtIndex:0];
        struct sockaddr_in * socketAddress = (struct sockaddr_in *) address.bytes;
        NSString* ipString = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
    
        NSString* portString = [NSNumber numberWithInteger:service.port].stringValue;
        socket = [[FastSocket alloc] initWithHost:ipString andPort:portString];
        [socket connect];
        [socket setTimeout:5];
    }
    
    [self flushToPrompt:socket];
    
    NSData* data = [command dataUsingEncoding:NSUTF8StringEncoding];
    long count = socket ? [socket sendBytes:data.bytes count:data.length] : _simulator->sendToShell(data.bytes, data.length);
    assert(count == data.length);
    return socket;
}

- (void)sendCommand:(NSString*)command andString:(NSString*) string fromService:(NSNetService*)service
{
    FastSocket* socket = [self sendCommand:command fromService:service];
    NSData* data = [string dataUsingEncoding:NSUTF8StringEncoding];
    long count = socket ? [socket sendBytes:data.bytes count:data.length] : _simulator->sendToShell(data.bytes, data.length);
    assert(count == data.length);
    [self flushToPrompt:socket];
}

- (NSString*)sendCommand:(NSString*)command fromService:(NSNetService*)service withTerminator:(char)terminator
{
    FastSocket* socket = [self sendCommand:command fromService:service];
    NSString* s = [self receiveFrom:socket toTerminator:terminator];
    return s;
}

- (NSArray*) fileListForDevice:(NSNetService*)service
{
    NSMutableArray* array = [[NSMutableArray alloc]init];
    NSString* fileString = [self sendCommand:@"ls\r\n" fromService:service withTerminator:'>'];
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
    
    return array;
}

- (void)reloadFilesWithBlock:(void (^)(FileList))handler
{
    [_fileList removeAllObjects];
    
    dispatch_async(_serialQueue, ^() {
        NSNetService* service = _currentDevice[@"service"];
        NSArray* fileList = [self fileListForDevice:service];
        for (NSDictionary* fileEntry in fileList) {
            [_fileList addObject:fileEntry];
        }
        
        [_fileList sortUsingComparator:^NSComparisonResult(NSDictionary* a, NSDictionary* b) {
            return [a[@"name"] compare:b[@"name"]];
        }];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            handler(_fileList);
        });
    });
}

- (void)reloadDevices
{
}

- (void)setFiles:(NSFileWrapper*)files
{
    _files = files;
    m8r::MacFS::setFiles(_files);
}

- (NSData*)contentsOfFile:(NSString*)name forDevice:(NSNetService*)service
{
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];
    
    NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    return data;
}

- (void)selectFile:(NSInteger)index
{
    NSString* name = _fileList[index][@"name"];
    
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
        NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];        
        NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.delegate setContents:data withName:name];
        });
    });
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

- (void)addFile:(NSString*)name withContents:(NSData*)contents toDevice:(NSNetService*)service
{
    NSString* contentString = [contents base64EncodedStringWithOptions:
                                                            NSDataBase64Encoding64CharacterLineLength |
                                                            NSDataBase64EncodingEndLineWithCarriageReturn | 
                                                            NSDataBase64EncodingEndLineWithLineFeed];
    NSString* command = [NSString stringWithFormat:@"put %@\r\n", name];
    contentString = [NSString stringWithFormat:@"%@\r\n\04\r\n", contentString];

    [self sendCommand:command andString:contentString fromService:service];
}

- (void)mirrorFiles
{
    if (!_currentDevice) {
        return;
    }
    
    NSNetService* service = _currentDevice[@"service"];
    NSArray* deviceFileList = [self fileListForDevice:service];
    NSArray* localFileList = [self fileListForDevice:nil];
    
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
        NSData* contents = [self contentsOfFile:name forDevice:nil];
        if ([self isFile:name inFileList:deviceFileList]) {
            // If contents are different, replace
            NSData* deviceContents = [self contentsOfFile:name forDevice:service];
            if (deviceContents.length == contents.length) {
                if (memcmp(deviceContents.bytes, contents.bytes, contents.length) == 0) {
                    continue;
                }
            }
        }

        [self addFile:name withContents:contents toDevice:service];
    }
}

- (void)addFile:(NSFileWrapper*)fileWrapper
{
    dispatch_async(_serialQueue, ^() {
        [self addFile:fileWrapper.preferredFilename withContents:fileWrapper.regularFileContents toDevice:_currentDevice[@"service"]];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate markDirty];
        });
    });
}

- (void)removeFile:(NSString*)name
{
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"rm %@\r\n", name];
        [self sendCommand:command fromService:service withTerminator:'>'];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate markDirty];
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

- (void)setDevice:(NSString*)device
{
    if (_logSocket) {
        _logSocket = nil;
        dispatch_sync(_logQueue, ^{ });
        _logQueue = nil;
    }

    _currentDevice = [self findService:device];
    if (!_currentDevice) {
        return;
    }
    
    NSNetService* service = _currentDevice[@"service"];
    if (service.addresses.count == 0) {
        return;
    }

    _logQueue = dispatch_queue_create("DeviceQueue", DISPATCH_QUEUE_SERIAL);

    NSData* address = [service.addresses objectAtIndex:0];
    struct sockaddr_in * socketAddress = (struct sockaddr_in *) address.bytes;
    NSString* ipString = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
    NSString* portString = [NSNumber numberWithInteger:service.port + 1].stringValue;
    
    _logSocket = [[FastSocket alloc]initWithHost:ipString andPort:portString];
    [_logSocket setTimeout:7200];

    dispatch_async(_logQueue, ^{
        [_logSocket connect];
        while (1) {
            char buffer[100];
            long count = [_logSocket receiveBytes:buffer limit:99];
            buffer[count] = '\0';
            [self outputMessage:[NSString stringWithUTF8String:buffer]];
        }
    });
}

- (void)reloadDeviceList
{
}

- (void)renameDevice:(NSString*)name
{
    dispatch_async(_serialQueue, ^() {
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"dev %@\r\n", name];
        
        NSString* s = [self sendCommand:command fromService:service withTerminator:'>'];
        NSLog(@"renameDevice returned '%@'", s);
    });
}

- (BOOL)canRun
{
    return _simulator->canRun();
}

- (BOOL)canStop
{
    return _simulator->canStop();
}

- (BOOL)canUpload
{
    return _currentDevice != nil;
}

- (BOOL)canSimulate
{
    return _simulator->canRun();
}

- (BOOL)canSaveBinary
{
    return _simulator->canSaveBinary();
}

- (void)buildFile:(NSString*) name
{
    _simulator->build(name.UTF8String);
}

- (void)run
{
    _simulator->run();
}

- (void)pause
{
    _simulator->pause();
}

- (void)stop
{
    _simulator->stop();
}

- (void)simulate
{
    _simulator->simulate();
}

- (void)saveBinary:(NSString*)filename
{
    NSString* name = [filename stringByDeletingPathExtension];
    name = [NSString stringWithFormat:@"%@.m8rb", name];
    std::vector<uint8_t> vector;
    if (!_simulator->exportBinary(vector)) {
        return;
    }
    
    NSData* data = [NSData dataWithBytes:&vector[0] length:vector.size()];
    [self addFile:name withContents:data toDevice:_currentDevice[@"service"]];
}

- (void)clearContents
{
    _simulator->clear();
}

- (void)importBinary:(const char*)filename
{
    _simulator->importBinary(filename);
}

- (void)exportBinary:(const char*)filename
{
    _simulator->exportBinary(filename);
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

int validateFileName(const char* name) {
    switch(m8r::Application::validateFileName(name)) {
        case m8r::Application::NameValidationType::Ok: return NameValidationOk;
        case m8r::Application::NameValidationType::BadLength: return NameValidationBadLength;
        case m8r::Application::NameValidationType::InvalidChar: return NameValidationInvalidChar;
    }
}
    
int validateBonjourName(const char* name)
{
    switch(m8r::Application::validateBonjourName(name)) {
        case m8r::Application::NameValidationType::Ok: return NameValidationOk;
        case m8r::Application::NameValidationType::BadLength: return NameValidationBadLength;
        case m8r::Application::NameValidationType::InvalidChar: return NameValidationInvalidChar;
    }
}

@end
