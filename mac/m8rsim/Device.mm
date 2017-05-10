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
#import <ctime>
#import <thread>
#import <chrono>
#import <MGSFragaria/MGSFragaria.h>
#import <malloc/malloc.h>

class DeviceSystemInterface;

@interface Device ()
{
    NSNetServiceBrowser* _netServiceBrowser;
    NSMutableArray<NSDictionary*>* _devices;
    NSDictionary* _currentDevice;
    FileList _fileList;
    NSFileWrapper* _files;

    m8r::MacFS* _fs;
    Simulator* _simulator;
    FastSocket* _logSocket;
    
    dispatch_queue_t _serialQueue;
    dispatch_queue_t _logQueue;
    
    BOOL _isBuild;
}

- (void)outputMessage:(NSString*)msg;
- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;
@end

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    return static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC);
}

uint32_t g_baselineMemoryUsed = 0;
#define MaxMemory 80000

malloc_zone_t* g_m8rzone = nullptr;

void* m8r::SystemInterface::alloc(MemoryType type, size_t size)
{
    return malloc_zone_malloc(g_m8rzone, size);
}

void m8r::SystemInterface::free(MemoryType, void* p)
{
    malloc_zone_free(g_m8rzone, p);
}

class DeviceSystemInterface : public m8r::SystemInterface
{
public:
    DeviceSystemInterface(Device* device) : _device(device), _gpio(device) { }
    
    virtual void vprintf(const char* s, va_list args) const override
    {
        NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
        [_device outputMessage:string];
    }
    
    virtual m8r::GPIOInterface& gpio() override { return _gpio; }
    virtual uint32_t freeMemory() const override
    {
        struct task_basic_info info;
        mach_msg_type_number_t size = sizeof(info);
        kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
        uint32_t used = (uint32_t) ((kerr == KERN_SUCCESS) ? info.resident_size : 0);
        if (g_baselineMemoryUsed == 0) {
            g_baselineMemoryUsed = used;
        }
        
        uint32_t memoryUsed = used - g_baselineMemoryUsed;
        return (memoryUsed > MaxMemory) ? 0 : MaxMemory - memoryUsed;
    }

private:
    class DeviceGPIOInterface : public m8r::GPIOInterface {
    public:
        DeviceGPIOInterface(Device* device) : _device(device) { }
        virtual ~DeviceGPIOInterface() { }

        virtual bool setPinMode(uint8_t pin, PinMode mode) override
        {
            if (!GPIOInterface::setPinMode(pin, mode)) {
                return false;
            }
            _pinio = (_pinio & ~(1 << pin)) | ((mode == PinMode::Output) ? (1 << pin) : 0);
            [_device updateGPIOState:_pinio withMode:_pinstate];
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
        Device* _device;
        
        // 0 = input, 1 = output
        uint32_t _pinio = 0;
        uint32_t _pinstate = 0;
    };
    
    Device* _device;
    DeviceGPIOInterface _gpio;
};

m8r::SystemInterface* _deviceSystemInterface = nullptr;

@implementation Device

- (instancetype)init
{
    self = [super init];
    if (self) {
        if (!g_m8rzone) {
            g_m8rzone = malloc_create_zone(MaxMemory, 0);
            malloc_set_zone_name(g_m8rzone, "m8rzone");
        }

        _fs = static_cast<m8r::MacFS*>(m8r::FS::createFS());
        
        _devices = [[NSMutableArray alloc] init];
        _fileList = [[NSMutableArray alloc] init];
        [self setFiles:[[NSFileWrapper alloc]initDirectoryWithFileWrappers:@{ }]];

        _deviceSystemInterface = new DeviceSystemInterface(self);
        _simulator = new Simulator(_fs, _deviceSystemInterface);
        
        _serialQueue = dispatch_queue_create("DeviceQueue", DISPATCH_QUEUE_SERIAL);

    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
        
        (void) m8r::IPAddr::myIPAddr();

        [self updateFreeMemory];
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
    delete _deviceSystemInterface;
    delete _fs;
}

- (void)outputMessage:(NSString*)msg
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate outputMessage:msg toBuild:_isBuild];
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
        if (![socket connect]) {
            _deviceSystemInterface->printf("**** Failed to open socket for command '%@'\n", command);
            return socket;
        }
        [socket setTimeout:5];
    }
    
    [self flushToPrompt:socket];
    
    NSData* data = [command dataUsingEncoding:NSUTF8StringEncoding];
    long count = socket ? [socket sendBytes:data.bytes count:data.length] : _simulator->sendToShell(data.bytes, data.length);
    assert(count == data.length);
    (void) count;
    return socket;
}

- (void)sendCommand:(NSString*)command andString:(NSString*) string fromService:(NSNetService*)service
{
    FastSocket* socket = [self sendCommand:command fromService:service];
    if (socket && ![socket isConnected]) {
        return;
    }
    NSData* data = [string dataUsingEncoding:NSUTF8StringEncoding];
    long count = socket ? [socket sendBytes:data.bytes count:data.length] : _simulator->sendToShell(data.bytes, data.length);
    assert(count == data.length);
    (void) count;
    [self flushToPrompt:socket];
    [socket close];
}

- (NSString*)sendCommand:(NSString*)command fromService:(NSNetService*)service withTerminator:(char)terminator
{
    FastSocket* socket = [self sendCommand:command fromService:service];
    if (socket && ![socket isConnected]) {
        return nil;
    }
    NSString* s = [self receiveFrom:socket toTerminator:terminator];
    [socket close];
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
    
    [self updateFreeMemory];
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
    _fs->setFiles(_files);
}

- (NSData*)contentsOfFile:(NSString*)name forDevice:(NSNetService*)service
{
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];
    
    NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    [self updateFreeMemory];
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
        [self updateFreeMemory];
    });
}

- (BOOL)saveFile:(NSString*)name withURLBase:(NSURL*)urlBase
{
    NSNetService* service = _currentDevice[@"service"];
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];
    NSURL* url = [NSURL URLWithString:name relativeToURL:urlBase];
    NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    [self updateFreeMemory];
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

- (void)addFile:(NSString*)name withContents:(NSData*)contents toDevice:(NSNetService*)service
{
    NSString* contentString = [contents base64EncodedStringWithOptions:
                                                            NSDataBase64Encoding64CharacterLineLength |
                                                            NSDataBase64EncodingEndLineWithCarriageReturn | 
                                                            NSDataBase64EncodingEndLineWithLineFeed];
    NSString* command = [NSString stringWithFormat:@"put %@\r\n", name];
    contentString = [NSString stringWithFormat:@"%@\r\n\04", contentString];

    [self sendCommand:command andString:contentString fromService:service];
    [self updateFreeMemory];
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
        [self updateFreeMemory];
    });
}

- (void)renameFileFrom:(NSString*)oldName to:(NSString*)newName
{
    NSString* __block command = [NSString stringWithFormat:@"mv %@ %@\r\n", oldName, newName];
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        [self sendCommand:command fromService:service withTerminator:'>'];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate markDirty];
        });
        [self updateFreeMemory];
    });
}

- (void)updateFreeMemory
{
    NSString* __block command = @"heap\r\n";
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* sizeString = [self sendCommand:command fromService:service withTerminator:'>'];
        NSArray* elements = [sizeString componentsSeparatedByString:@":"];
        if (elements.count != 2 || ![elements[0] isEqualToString:@"heap"]) {
            return;
        }
        NSUInteger size = [[elements objectAtIndex:1] intValue];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_delegate setFreeMemory:size];
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
    [self updateFreeMemory];

    if (!_currentDevice) {
        return;
    }
    
    NSNetService* service = _currentDevice[@"service"];
    if (service.addresses.count == 0) {
        return;
    }

    _logQueue = dispatch_queue_create("LogQueue", DISPATCH_QUEUE_SERIAL);

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
            if (count == 0) {
                break;
            }
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
    [self updateFreeMemory];
}

- (BOOL)canRun
{
    return _simulator->canRun();
}

- (BOOL)canBuild
{
    return _currentDevice == nil;
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

- (NSArray*)buildFile:(NSString*) name withDebug:(BOOL)debug
{
    _isBuild = YES;
    const m8r::ErrorList* errors = _simulator->build(name.UTF8String, debug);
    
    if (!errors) {
        return nil;
    }
    
    NSMutableArray* errorArray = [[NSMutableArray alloc] init];
    for (const auto& entry : *errors) {
        SMLSyntaxError *syntaxError = [SMLSyntaxError new];
        syntaxError.description = [NSString stringWithUTF8String:entry._description];
        syntaxError.line = entry._lineno;
        syntaxError.character = entry._charno;
        syntaxError.length = entry._length;
        [errorArray addObject:syntaxError];
    }
    
    [self updateFreeMemory];
    return errorArray;
}

- (void)runFile:(NSString*) name withDebug:(BOOL)debug
{
    _isBuild = NO;
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        if (debug) {
            NSString* command = [NSString stringWithFormat:@"debug\r\n"];
            [self sendCommand:command fromService:service withTerminator:'>'];
        }
        
        NSString* command = [NSString stringWithFormat:@"run %@\r\n", name];
        [self sendCommand:command fromService:service withTerminator:'>'];
        dispatch_async(dispatch_get_main_queue(), ^{
        });
        [self updateFreeMemory];
    });
}

- (void)pause
{
    _simulator->pause();
}

- (void)stop
{
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"stop\r\n"];
        [self sendCommand:command fromService:service withTerminator:'>'];
        dispatch_async(dispatch_get_main_queue(), ^{
        });
        [self updateFreeMemory];
    });
}

- (void)simulate
{
    _simulator->simulate();
}

- (BOOL)saveChangedFiles
{
    // FIXME: Implement
    return _currentDevice != nil;
}

- (BOOL)isDeviceFile
{
    return _currentDevice != nil;
}

- (void)clearContents
{
    _simulator->clear();
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
