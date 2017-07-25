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

@end

@implementation m8rsim_xpc

- (void)initWithReply:(void (^)(NSInteger))reply
{
    reply(0);
}

- (void)setPort:(NSUInteger)port withReply:(void (^)(NSInteger))reply
{
    _simulator = [[Simulator alloc] initWithPort:port];
    reply(_simulator.status);
}

- (void)setFiles:(NSURL*)files withReply:(void (^)(NSInteger))reply
{
    reply([_simulator setFiles:files]);
}

@end
