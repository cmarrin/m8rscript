//
//  Simulator.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class Document;

@interface Simulator : NSObject

- (instancetype)initWithDocument:(Document*) document;

- (BOOL)canRun;
- (BOOL)canStop;
- (void)importBinary:(const char*)filename;
- (void)exportBinary:(const char*)filename;

@end

