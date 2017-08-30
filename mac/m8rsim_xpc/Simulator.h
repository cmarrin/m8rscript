//
//  Simulator.h
//  m8rscript
//
//  Created by Chris Marrin on 7/6/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface Simulator : NSObject

@property (readonly) NSInteger status;

- (instancetype)initWithPort:(NSUInteger)port connection:(NSXPCConnection*)xpc;
- (NSInteger)setFiles:(NSURL*)files;

- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;

@end
