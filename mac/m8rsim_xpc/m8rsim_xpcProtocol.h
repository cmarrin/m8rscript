//
//  m8rsim_xpcProtocol.h
//  m8rsim_xpc
//
//  Created by Chris Marrin on 7/4/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#import <Foundation/Foundation.h>

// The protocol that this service will vend as its API. This header file will also need to be visible to the process hosting the service.
@protocol m8rsim_xpcProtocol

- (void)initWithPort:(NSUInteger)port withReply:(void (^)(NSInteger))reply;
- (void)setFiles:(NSURL*)files withReply:(void (^)(NSInteger))reply;
    
@end

@protocol m8rsim_xpcResponse

- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;

@end
