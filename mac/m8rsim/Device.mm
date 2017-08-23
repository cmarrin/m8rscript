//
//  Device.mm
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Device.h"

#import "FastSocket.h"
#import "m8rsim_xpcProtocol.h"
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>
#import <ctime>
#import <thread>
#import <chrono>
#import <MGSFragaria/MGSFragaria.h>

#define LocalPort 2222

@interface Device ()
{
    NSNetServiceBrowser* _netServiceBrowser;
    NSMutableArray<NSDictionary*>* _devices;
    NSDictionary* _currentDevice;
    FileList _fileList;
    
    NSXPCConnection* _simulator;

    FastSocket* _logSocket;
    
    dispatch_queue_t _serialQueue;
    dispatch_queue_t _logQueue;

    BOOL _isBuild;
}

- (void)outputMessage:(NSString*)msg, ...;
- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;
- (void)updateMemoryInfo;

@end

@implementation Device

- (instancetype)init
{
    self = [super init];
    if (self) {
        _devices = [[NSMutableArray alloc] init];
        _fileList = [[NSMutableArray alloc] init];

        _simulator = [[NSXPCConnection alloc] initWithServiceName:@"org.marrin.m8rsim-xpc"];
        _simulator.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(m8rsim_xpcProtocol)];
        [_simulator resume];
        
//        [[_simulator remoteObjectProxy] initWithReply:^(NSInteger status) {
//            if (status < 0) {
//                [self outputMessage:@"**** Failed to create xpc, error: %d\n", status];
//                [_simulator invalidate];
//                _simulator = nil;
//            } else {
//                [[_simulator remoteObjectProxy] setPort:LocalPort withReply:^(NSInteger status) {
//                    if (status < 0) {
//                        [self outputMessage:@"**** Failed to set xpc LocalPort, error: %d\n", status];
//                        [_simulator invalidate];
//                        _simulator = nil;
//                    } else {
//                        [self.delegate setDevice:@""];
//                    }
//                }];
//            }
//        }];

        _serialQueue = dispatch_queue_create("DeviceQueue", DISPATCH_QUEUE_SERIAL);
    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
    }
    return self;
}

- (void)dealloc
{
     [_simulator invalidate];
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

- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode
{
    [self.delegate updateGPIOState:state withMode:mode];
}

- (void)flushToPrompt:(FastSocket*) socket
{
    while(1) {
        char c;
        long count = [socket receiveBytes:&c count:1];
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
        long count = [socket receiveBytes:c count:1];
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
    NSString* ipString = @"127.0.0.1";
    NSInteger port = LocalPort;
    
    if (service) {
        if (service.addresses.count == 0) {
            return nil;
        }
        NSData* address = [service.addresses objectAtIndex:0];
        struct sockaddr_in * socketAddress = (struct sockaddr_in *) address.bytes;
        ipString = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
        port = service.port;
    } else if (!_simulator) {
        return nil;
    }
    
    NSString* portString = [NSNumber numberWithInteger:port].stringValue;
    socket = [[FastSocket alloc] initWithHost:ipString andPort:portString];
    if (![socket connect]) {
        [self outputMessage:@"**** Failed to open socket for command '%@'\n", command];
        return socket;
    }
    [socket setTimeout:5];
    
    [self flushToPrompt:socket];
    
    NSData* data = [command dataUsingEncoding:NSUTF8StringEncoding];
    long count = [socket sendBytes:data.bytes count:data.length];
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
    long count = [socket sendBytes:data.bytes count:data.length];
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
    
    [self updateMemoryInfo];
    return array;
}

- (void)reloadFilesWithBlock:(void (^)(FileList))handler
{
    NSNetService* service = _currentDevice[@"service"];
    NSArray* fileList = [self fileListForDevice:service];
    for (NSDictionary* fileEntry in fileList) {
        [_fileList addObject:fileEntry];
    }
    
    [_fileList sortUsingComparator:^NSComparisonResult(NSDictionary* a, NSDictionary* b) {
        return [a[@"name"] compare:b[@"name"]];
    }];
    
    handler(_fileList);
}

- (void)reloadFilesWithURL:(NSURL*)url withBlock:(void (^)(FileList))handler
{
    [_fileList removeAllObjects];
    [[_simulator remoteObjectProxy] initWithPort:LocalPort files:url];
    usleep(1000000);
    [self reloadFilesWithBlock:handler];
}

- (void)reloadDevices
{
}

- (NSData*)contentsOfFile:(NSString*)name forDevice:(NSNetService*)service
{
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];
    
    NSData* data = [[NSData alloc]initWithBase64EncodedString:fileContents options:NSDataBase64DecodingIgnoreUnknownCharacters];
    [self updateMemoryInfo];
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
        [self updateMemoryInfo];
    });
}

- (BOOL)saveFile:(NSString*)name withURLBase:(NSURL*)urlBase
{
    NSNetService* service = _currentDevice[@"service"];
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];
    NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];
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

- (void)addFile:(NSString*)name withContents:(NSData*)contents toDevice:(NSNetService*)service
{
    NSString* contentString = [contents base64EncodedStringWithOptions:
                                                            NSDataBase64Encoding64CharacterLineLength |
                                                            NSDataBase64EncodingEndLineWithCarriageReturn | 
                                                            NSDataBase64EncodingEndLineWithLineFeed];
    NSString* command = [NSString stringWithFormat:@"put %@\r\n", name];
    contentString = [NSString stringWithFormat:@"%@\r\n\04", contentString];

    [self sendCommand:command andString:contentString fromService:service];
    [self updateMemoryInfo];
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
        [self updateMemoryInfo];
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
        [self updateMemoryInfo];
    });
}

- (void)updateMemoryInfo
{
    NSString* __block command = @"heap\r\n";
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* sizeString = [self sendCommand:command fromService:service withTerminator:'>'];
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
    if (_logSocket) {
        _logSocket = nil;
        dispatch_sync(_logQueue, ^{ });
        _logQueue = nil;
    }

    _currentDevice = [self findService:device];
    if (!_currentDevice && !_simulator) {
        [self outputMessage:@"**** No xpc connection\n"];
        return NO;
    }

    NSString* ipString = @"127.0.0.1";
    NSUInteger port = LocalPort + 1;
    
    if (_currentDevice) {
        NSNetService* service = _currentDevice[@"service"];
        if (service.addresses.count == 0) {
            return NO;
        }

        NSData* address = [service.addresses objectAtIndex:0];
        struct sockaddr_in * socketAddress = (struct sockaddr_in *) address.bytes;
        ipString = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
        port = service.port + 1;
    }
    
    NSString* portString = [NSNumber numberWithInteger:port].stringValue;
    
    _logQueue = dispatch_queue_create("LogQueue", DISPATCH_QUEUE_SERIAL);
    
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
    
    [_delegate reloadFiles];
    
    return YES;
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
    NSNetService* service = _currentDevice[@"service"];
    if (debug) {
        NSString* command = [NSString stringWithFormat:@"debug\r\n"];
        [self sendCommand:command fromService:service withTerminator:'>'];
    }
    
    NSString* command = [NSString stringWithFormat:@"build %@\r\n", name];
    NSString* errors = [self sendCommand:command fromService:service withTerminator:'>'];
    [self updateMemoryInfo];
    
    if (!errors || ![errors length]) {
        return nil;
    }
    
    NSMutableArray* errorArray = [[NSMutableArray alloc] init];

    NSArray* lines = [errors componentsSeparatedByString:@"\n"];

    for (NSString* line in lines) {
        NSArray* elements = [line componentsSeparatedByString:@":"];
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
        [self updateMemoryInfo];
    });
}

- (void)pause
{
    // FIXME: Implement
}

- (void)stop
{
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"stop\r\n"];
        [self sendCommand:command fromService:service withTerminator:'>'];
        dispatch_async(dispatch_get_main_queue(), ^{
        });
        [self updateMemoryInfo];
    });
}

- (void)simulate
{
    // FIXME: Implement
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
    dispatch_async(_serialQueue, ^() {        
        NSNetService* service = _currentDevice[@"service"];
        NSString* command = [NSString stringWithFormat:@"clear\r\n"];
        [self sendCommand:command fromService:service withTerminator:'>'];
        dispatch_async(dispatch_get_main_queue(), ^{
        });
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
