//
//  Document.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface Document : NSDocument

typedef NS_ENUM(NSInteger, OutputType) { CTBuild, CTConsole };
- (void)clearOutput:(OutputType)output;
- (void)outputMessage:(NSString*) message to:(OutputType) output;
- (void)markDirty;
- (void)setSource:(NSString*)source;

@end

