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

- (void)initWithPort:(NSUInteger)port withReply:(void (^)(NSInteger))reply
{
    _simulator = [[Simulator alloc] initWithPort:port];
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
