//
//  Document.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Device.h"

@interface Document : NSDocument <DeviceDelegate>

typedef NS_ENUM(NSInteger, OutputType) { CTBuild, CTConsole };
- (void)clearOutput:(OutputType)output;

- (void)setSource:(NSString*)source;
- (void)setImage:(NSImage*)image;
- (void)selectFile:(NSInteger)index;
- (void)addFile:(NSFileWrapper*)file;
- (void)removeFile:(NSString*)name;
- (void)reloadFiles;
- (void)setDevice:(NSString*)name;

@end

