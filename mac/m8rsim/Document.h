//
//  Document.h
//  m8rsim
//
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#import <Cocoa/Cocoa.h>

#import "Device.h"

@interface Document : NSDocument <DeviceDelegate>

typedef NS_ENUM(NSInteger, OutputType) { CTBuild, CTConsole };
- (void)clearOutput:(OutputType)output;

- (void)setImage:(NSImage*)image;
- (void)selectFile:(NSInteger)index;
- (void)addFile:(NSFileWrapper*)file;
- (void)removeFile:(NSString*)name;
- (void)renameFileFrom:(NSString*)oldName to:(NSString*)newName;
- (void)reloadFiles;
- (void)setDevice:(NSString*)name;
- (BOOL)saveFile:(NSString*)name withURLBase:(NSURL*)urlBase;
- (void) setFreeMemory:(NSUInteger)size numObj:(NSUInteger)obj numStr:(NSUInteger)str numOther:(NSUInteger)oth;

@end

