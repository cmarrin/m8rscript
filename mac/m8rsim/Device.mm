//
//  Device.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Device.h"

#import "FastSocket.h"
#import "Simulator.h"
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
}

- (void)outputMessage:(NSString*)msg;
- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;
@end

class DeviceSystemInterface : public m8r::SystemInterface
{
public:
    DeviceSystemInterface(Device* device) : _device(device) { }
    
    virtual void printf(const char* s, ...) const override
    {
        va_list args;
        va_start(args, s);
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
        _system = new DeviceSystemInterface(self);
        _simulator = new Simulator(_system);
    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
     }
    return self;
}

- (void)dealloc
{
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

- (FastSocket*)sendCommand:(NSString*)command fromService:(NSNetService*)service asBinary:(BOOL)binary
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
    
    if (binary) {
        // Set to binary mode
        if (socket) {
            [socket sendBytes:(const void*)@"b\r\n" count:3];
        } else {
            _simulator->sendToShell("b\r\n", 3);
        }
        [NSThread sleepForTimeInterval:1];
        [self flushToPrompt:socket];
    }
    
    NSData* data = [command dataUsingEncoding:NSUTF8StringEncoding];
    long count = socket ? [socket sendBytes:data.bytes count:data.length] : _simulator->sendToShell(data.bytes, data.length);
    assert(count == data.length);
    return socket;
}

- (void)sendCommand:(NSString*)command andString:(NSString*) string fromService:(NSNetService*)service asBinary:(BOOL)binary
{
    FastSocket* socket = [self sendCommand:command fromService:service asBinary:binary];
    NSData* data = [string dataUsingEncoding:NSUTF8StringEncoding];
    long count = socket ? [socket sendBytes:data.bytes count:data.length] : _simulator->sendToShell(data.bytes, data.length);
    assert(count == data.length);
    [self flushToPrompt:socket];
}

- (NSString*)sendCommand:(NSString*)command fromService:(NSNetService*)service asBinary:(BOOL)binary withTerminator:(char)terminator
{
    FastSocket* socket = [self sendCommand:command fromService:service asBinary:binary];
    NSString* s = [self receiveFrom:socket toTerminator:terminator];
    return s;
}

- (void)reloadFilesWithBlock:(void (^)(FileList))handler
{
    [_fileList removeAllObjects];
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
//        if (!_currentDevice) {
//            for (NSString* name in _files.fileWrappers) {
//                NSFileWrapper* file = _files.fileWrappers[name];
//                if (file && file.regularFile) {
//                    [_fileList addObject:@{ @"name" : name, @"size" : @(_files.fileWrappers[name].regularFileContents.length) }];
//                }
//            }
//        } else {
            // load files from the device
            NSNetService* service = _currentDevice[@"service"];
            NSString* fileString = [self sendCommand:@"ls\r\n" fromService:service asBinary:YES withTerminator:'>'];
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
            
                [_fileList addObject:@{ @"name" : name, @"size" : size }];
            }
//        }
        
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
}

- (void)selectFile:(NSInteger)index
{
    NSString* name = _fileList[index][@"name"];
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {        
        if (_currentDevice) {
            NSNetService* service = _currentDevice[@"service"];
            NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
            NSString* fileContents = [self sendCommand:command fromService:service asBinary:YES withTerminator:'\04'];
        
            dispatch_async(dispatch_get_main_queue(), ^{
                [self.delegate setSource:fileContents];
            });
        } else {
            NSString* content = [[NSString alloc] initWithData:_files.fileWrappers[name].regularFileContents encoding:NSUTF8StringEncoding];
            if (content) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self.delegate setSource:content];
                });
            } else {
                NSImage* image = [[NSImage alloc] initWithData:_files.fileWrappers[name].regularFileContents];
                if (image) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [self.delegate setImage:image];
                    });
                }
            }
        }
    });
}

- (void)addFile:(NSFileWrapper*)fileWrapper
{
    NSString* name = fileWrapper.preferredFilename;
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {        
        if (_currentDevice) {
            NSNetService* service = _currentDevice[@"service"];
            NSString* contents = [fileWrapper.regularFileContents base64EncodedStringWithOptions:
                                                                    NSDataBase64Encoding64CharacterLineLength |
                                                                    NSDataBase64EncodingEndLineWithCarriageReturn | 
                                                                    NSDataBase64EncodingEndLineWithLineFeed];
            NSString* command = [NSString stringWithFormat:@"put %@\r\n", fileWrapper.preferredFilename];
            contents = [NSString stringWithFormat:@"%@\r\n\04\r\n", contents];

            [self sendCommand:command andString:contents fromService:service asBinary:YES];
        } else {
            if (!_files) {
                _files = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ }];
            }
            if (_files.fileWrappers[name]) {
                [_files removeFileWrapper:_files.fileWrappers[name]];
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                [_delegate markDirty];
            });
            [_files addFileWrapper:fileWrapper];
        }
    });
}

- (void)removeFile:(NSString*)name
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {        
        if (_currentDevice) {
            NSNetService* service = _currentDevice[@"service"];
            NSString* command = [NSString stringWithFormat:@"rm %@\r\n", name];
            [self sendCommand:command fromService:service asBinary:NO withTerminator:'>'];
        } else {
            NSFileWrapper* d = _files.fileWrappers[name];
            if (d) {
                [_files removeFileWrapper:d];
                [_delegate markDirty];
            }
        }
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
    _currentDevice = [self findService:device];
}

- (void)reloadDeviceList
{
}

- (void)renameDevice:(NSString*)name
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"dev %@\r\n", name];
        
        NSString* s = [self sendCommand:command fromService:service asBinary:NO withTerminator:'>'];
        NSLog(@"renameDevice returned '%@'", s);
    });
}

- (void)upload
{
}

- (BOOL)canRun
{
    return YES;
}

- (BOOL)canStop
{
    return YES;
}

- (void)build:(const char*) source withName:(NSString*) name
{
    _simulator->build(source, name.UTF8String);
}

- (void)run
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        _simulator->run();
    });
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
