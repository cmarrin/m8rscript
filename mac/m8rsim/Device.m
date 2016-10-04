//
//  Device.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Device.h"

#import "FastSocket.h"
#import "Engine.h"
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>

@interface Device ()
{
    NSNetServiceBrowser* _netServiceBrowser;
    NSMutableArray<NSDictionary*>* _devices;
    NSDictionary* _currentDevice;
    FileList _fileList;
}

@end

@implementation Device

- (instancetype)init
{
    self = [super init];
    if (self) {
        _devices = [[NSMutableArray alloc] init];
        _fileList = [[NSMutableArray alloc] init];
    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
     }
    return self;
}

- (void)dealloc
{
}

static void flushToPrompt(FastSocket* socket)
{
    while(1) {
        char c;
        long count = [socket receiveBytes:&c count:1];
        if (count != 1 || c == '>') {
            break;
        }
    }
}

static NSString* receiveToTerminator(FastSocket* socket, char terminator)
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
    if (service.addresses.count == 0) {
        return nil;
    }
    NSData* address = [service.addresses objectAtIndex:0];
    struct sockaddr_in * socketAddress = (struct sockaddr_in *) address.bytes;
    NSString* ipString = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
    
    NSString* portString = [NSNumber numberWithInteger:service.port].stringValue;
    FastSocket* socket = [[FastSocket alloc] initWithHost:ipString andPort:portString];
    [socket connect];
    [socket setTimeout:5];
    flushToPrompt(socket);
    
    // Set to binary mode
    long count = [socket sendBytes:(const void*)@"b\r\n" count:3];
    
    NSData* data = [command dataUsingEncoding:NSUTF8StringEncoding];
    count = [socket sendBytes:data.bytes count:data.length];
    assert(count == data.length);
    return socket;
}

- (void)sendCommand:(NSString*)command andString:(NSString*) string fromService:(NSNetService*)service
{
    FastSocket* socket = [self sendCommand:command fromService:service];
    if (!socket) {
        return;
    }
    NSData* data = [string dataUsingEncoding:NSUTF8StringEncoding];
    long count = [socket sendBytes:data.bytes count:data.length];
    assert(count == data.length);
    flushToPrompt(socket);
}

- (NSString*)sendCommand:(NSString*)command fromService:(NSNetService*)service withTerminator:(char)terminator
{
    FastSocket* socket = [self sendCommand:command fromService:service];
    if (!socket) {
        return nil;
    }
    NSString* s = receiveToTerminator(socket, terminator);
    return s;
}

- (void)reloadFilesWithBlock:(void (^)(FileList))handler
{
    [_fileList removeAllObjects];
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        // load files from the device
        NSNetService* service = _currentDevice[@"service"];
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
            
            [_fileList addObject:@{ @"name" : name, @"size" : size }];
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

- (void)selectFile:(NSString*)name withBlock:(void (^)(NSString*))handler
{
    NSNetService* service = _currentDevice[@"service"];
    NSString* command = [NSString stringWithFormat:@"get %@\r\n", name];

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        NSString* fileContents = [self sendCommand:command fromService:service withTerminator:'\04'];
        dispatch_async(dispatch_get_main_queue(), ^{
            handler(fileContents);
        });
    });
}

- (void)addFile:(NSFileWrapper*)fileWrapper
{
    NSNetService* service = _currentDevice[@"service"];
    NSString* contents = [[NSString alloc] initWithData:fileWrapper.regularFileContents encoding:NSUTF8StringEncoding]; 
    NSString* command = [NSString stringWithFormat:@"put %@\r\n", fileWrapper.preferredFilename];
    contents = [NSString stringWithFormat:@"%@\r\n\04\r\n", contents];

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        [self sendCommand:command andString:contents fromService:service];
    });
}

- (void)removeFile:(NSString*)name
{
    NSNetService* service = _currentDevice[@"service"];
    NSString* command = [NSString stringWithFormat:@"rm %@\r\n", name];
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        [self sendCommand:command fromService:service withTerminator:'>'];
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
        
        NSString* s = [self sendCommand:command fromService:service withTerminator:'>'];
        NSLog(@"renameDevice returned '%@'", s);
    });
}

- (void)upload
{
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
    [_dataSource addDevice:hostName];
}

- (void)netService:(NSNetService *)sender 
     didNotResolve:(NSDictionary<NSString *,NSNumber *> *)errorDict
{
    NSLog(@"********* Did not resolve: %@\n", errorDict);
}

@end
