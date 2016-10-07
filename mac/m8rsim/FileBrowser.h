//
//  FileBrowser.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class Document;
@class Device;

@interface FileBrowser : NSViewController

@property (readonly, strong) NSFileWrapper* files;

- (instancetype)initWithDocument:(Document*) document;

- (void)renameDevice;
- (void)upload;
- (void)addFiles;
- (void)removeFiles;
- (void)setFiles:(NSFileWrapper*)files;
- (BOOL)isFileSourceLocal;

- (void)reloadFilesForDevice:(Device*)device;

@end

