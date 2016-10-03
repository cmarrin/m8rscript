//
//  Device.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

typedef NSMutableArray<NSDictionary*>* FileList;

@protocol DeviceDataSource

- (void)clearDeviceList;
- (void)addDevice:(NSString*)name;

@end

@interface Device : NSObject

@property (weak) id <DeviceDataSource> dataSource;
@property (readonly) NSDictionary* currentDevice;

- (void)reloadFilesWithBlock:(void (^)(FileList))handler;
- (void)selectFile:(NSString*)name withBlock:(void (^)(NSString*))handler;
- (void)addFile:(NSFileWrapper*)fileWrapper;
- (NSDictionary*) findService:(NSString*)hostname;
- (void)setDevice:(NSString*)device;
- (void)renameDevice:(NSString*)name;
- (void)removeFile:(NSString*)name;

@end

