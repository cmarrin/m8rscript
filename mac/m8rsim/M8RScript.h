//
//  M8RScript.h
//  m8rscript
//
//  Created by Chris Marrin on 6/28/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

#ifndef M8RScript_h
#define M8RScript_h

#import <Cocoa/Cocoa.h>

@class Document;

@interface M8RScript

- (instancetype)initWithDocument:(Document*) document;

- (BOOL)canRun;
- (BOOL)canStop;
- (BOOL)canPause;

- (void)build:(NSString*)string;
- (void)run;
- (void)pause;
- (void)stop;

@end


#endif /* M8RScript_h */
