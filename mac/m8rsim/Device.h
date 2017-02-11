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
- (void)setContents:(NSData*)contents withName:(NSString*)name;
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

- (void)mirrorFiles;
- (void)setFiles:(NSFileWrapper*)files;
- (void)selectFile:(NSInteger)index;
- (void)addFile:(NSFileWrapper*)fileWrapper;
- (void)removeFile:(NSString*)name;
- (void)renameFileFrom:(NSString*)oldName to:(NSString*)newName;
- (void)setDevice:(NSString*)device;
- (void)clearContents;

- (BOOL)canRun;
- (BOOL)canStop;
- (BOOL)canUpload;
- (BOOL)canSimulate;
- (BOOL)canSaveBinary;

- (void)importBinary:(const char*)filename;
- (void)exportBinary:(const char*)filename;
- (void)buildFile:(NSString*) name withDebug:(BOOL)debug;
- (void)runFile:(NSString*) name;
- (void)pause;
- (void)stop;
- (void)simulate;
- (void)saveBinary:(NSString*)filename;


@end

