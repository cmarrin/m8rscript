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

typedef NSMutableArray<NSDictionary*>* FileList;

@protocol DeviceDelegate

- (void)addDevice:(NSString*)name;
- (void)setDevice:(NSString*)name;
- (void)setContents:(NSData*)contents withName:(NSString*)name;
- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;
- (void)outputMessage:(NSString*) message toBuild:(BOOL) build;
- (void)reloadFiles;
- (void) setFreeMemory:(NSUInteger)size numObj:(NSUInteger)obj numStr:(NSUInteger)str numOther:(NSUInteger)oth;
- (void)markDirty;

@end

@interface Device : NSObject

@property (weak) id <DeviceDelegate> delegate;
@property (readonly) NSDictionary* currentDevice;

- (void)reloadFilesWithURL:(NSURL*)url withBlock:(void (^)(FileList))handler;
- (NSDictionary*) findService:(NSString*)hostname;
- (void)renameDevice:(NSString*)name;

- (void)mirrorFiles;
- (void)selectFile:(NSInteger)index;
- (void)addFile:(NSFileWrapper*)fileWrapper;
- (void)removeFile:(NSString*)name;
- (void)renameFileFrom:(NSString*)oldName to:(NSString*)newName;
- (BOOL)setDevice:(NSString*)device;
- (void)clearContents;
- (BOOL)saveFile:(NSString*)name withURLBase:(NSURL*)urlBase;

- (BOOL)canRun;
- (BOOL)canBuild;
- (BOOL)canStop;
- (BOOL)canUpload;
- (BOOL)canSimulate;

- (NSArray*)buildFile:(NSString*) name withDebug:(BOOL)debug;
- (void)runFile:(NSString*) name withDebug:(BOOL)debug;
- (void)pause;
- (void)stop;
- (void)simulate;

- (BOOL)saveChangedFiles;
- (BOOL)isDeviceFile;

@end

