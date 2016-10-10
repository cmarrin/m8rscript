//
//  Device.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define NameValidationOk 0
#define NameValidationBadLength 1
#define NameValidationInvalidChar 2

#ifdef __cplusplus
extern "C" {
#endif
int validateFileName(const char*);
int validateBonjourName(const char*);
#ifdef __cplusplus
}
#endif

typedef NSMutableArray<NSDictionary*>* FileList;

@protocol DeviceDelegate

- (void)addDevice:(NSString*)name;
- (void)setSource:(NSString*)source withName:(NSString*)name;
- (void)setImage:(NSImage*)image;
- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;
- (void)outputMessage:(NSString*) message toBuild:(BOOL) build;
- (void)markDirty;

@end

@interface Device : NSObject

@property (weak) id <DeviceDelegate> delegate;
@property (readonly) NSDictionary* currentDevice;
@property (readonly, strong) NSFileWrapper* files;

- (void)reloadFilesWithBlock:(void (^)(FileList))handler;
- (NSDictionary*) findService:(NSString*)hostname;
- (void)renameDevice:(NSString*)name;

- (void)setFiles:(NSFileWrapper*)files;
- (void)selectFile:(NSInteger)index;
- (void)addFile:(NSFileWrapper*)fileWrapper;
- (void)removeFile:(NSString*)name;
- (void)setDevice:(NSString*)device;
- (BOOL)canRun;
- (BOOL)canStop;

- (void)importBinary:(const char*)filename;
- (void)exportBinary:(const char*)filename;
- (void)build:(const char*) source withName:(NSString*) name;
- (void)run;
- (void)pause;
- (void)stop;
- (void)simulate;


@end

