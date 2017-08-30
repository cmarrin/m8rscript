//
//  m8rsim_xpc.m
//  m8rsim_xpc
//
//  Created by Chris Marrin on 7/4/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#import "m8rsim_xpc.h"

#import "Simulator.h"

@interface m8rsim_xpc ()
{
    Simulator* _simulator;
}

@property (weak) NSXPCConnection *xpcConnection;

@end

@implementation m8rsim_xpc

- (BOOL)listener:(NSXPCListener *)listener shouldAcceptNewConnection:(NSXPCConnection *)newConnection {
    // This method is where the NSXPCListener configures, accepts, and resumes a new incoming NSXPCConnection.
    
    // Configure the connection.
    // First, set the interface that the exported object implements.
    newConnection.exportedInterface = [NSXPCInterface interfaceWithProtocol:@protocol(m8rsim_xpcProtocol)];
    newConnection.exportedObject = self;
    
    newConnection.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol: @protocol(m8rsim_xpcResponse)];
    
    self.xpcConnection = newConnection;

    // Resuming the connection allows the system to deliver more incoming messages.
    [newConnection resume];
    
    // Returning YES from this method tells the system that you have accepted this connection. If you want to reject the connection for some reason, call -invalidate on the connection and return NO.
    return YES;
}

- (void)initWithPort:(NSUInteger)port withReply:(void (^)(NSInteger))reply
{
    _simulator = [[Simulator alloc] initWithPort:port connection:self.xpcConnection];
    reply(0);
}

- (void)setFiles:(NSURL*)files withReply:(void (^)(NSInteger))reply;
{
    if (!_simulator) {
        reply(-1);
    }
    [_simulator setFiles:files];
    reply(0);
}


@end
