//
//  Device.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

typedef NSMutableArray<NSDictionary*>* FileList;

@protocol DeviceDelegate

- (void)clearDeviceList;
- (void)addDevice:(NSString*)name;
- (void)setSource:(NSString*)source;
- (void)setImage:(NSImage*)image;

@end

@interface Device : NSObject

@property (weak) id <DeviceDelegate> delegate;
@property (readonly) NSDictionary* currentDevice;

- (void)reloadFilesWithBlock:(void (^)(FileList))handler;
- (NSDictionary*) findService:(NSString*)hostname;
- (void)setDevice:(NSString*)device;
- (void)renameDevice:(NSString*)name;
- (void)removeFile:(NSString*)name;

- (void)selectFile:(NSInteger)index withBlock:(void (^)())handler;
- (void)addFile:(NSFileWrapper*)fileWrapper withBlock:(void (^)())handler;

@end

