//
//  FileBrowser.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "FileBrowser.h"

#import "Document.h"
#import "FastSocket.h"
#import "Engine.h"
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>

@interface FileBrowser ()
{
    __weak IBOutlet NSProgressIndicator *busyIndicator;
    __weak IBOutlet NSTableView *fileListView;
    __weak IBOutlet NSPopUpButton *fileSourceButton;

    Document* _document;
    Device* _device;

    FileList _localFileList;
    FileList _deviceFileList;
    NSFileWrapper* _files;

    NSTextField* renameDeviceTextField;
}

@end

@implementation FileBrowser

- (instancetype)initWithDocument:(Document*) document {
    self = [super init];
    if (self) {
        _document = document;
        _localFileList = [[NSMutableArray alloc] init];
     }
    return self;
}

- (void)dealloc
{
}

- (void)awakeFromNib
{    
    renameDeviceTextField = [[NSTextField alloc]initWithFrame:NSMakeRect(0, 0, 240, 22)];

    // Clear the fileSourceButton
    assert(fileSourceButton.numberOfItems > 0);
    [fileSourceButton selectItemAtIndex:0];
    while (fileSourceButton.numberOfItems > 1) {
        [fileSourceButton removeItemAtIndex:1];
    }
    
    _device = [[Device alloc] init];
    _device.dataSource = self;
}

- (BOOL)isFileSourceLocal
{
    return [fileSourceButton.selectedItem.title isEqualToString:@"Local Files"];
}

- (FileList)currentFileList
{
    return self.isFileSourceLocal ? _localFileList : _deviceFileList;
}

- (BOOL)fileListContains:(NSString*)name
{
    for (NSDictionary* entry in self.currentFileList) {
        if ([entry[@"name"] isEqualToString:name]) {
            return YES;
        }
    }
    return NO;
}

- (void)setFiles:(NSFileWrapper*)files
{
    _files = files;
    [self reloadFiles];
}

- (NSFileWrapper*)filesFileWrapper
{
    if (!_files) {
        [_document markDirty];
        _files = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ }];
    }
    return _files;
}

- (void)reloadFiles
{
    if (!self.isFileSourceLocal) {
        [busyIndicator setHidden:NO];
        [busyIndicator startAnimation:nil];
        [_device reloadFilesWithBlock:^(FileList fileList) {
            _deviceFileList = fileList;
            [fileListView reloadData];
            [busyIndicator stopAnimation:nil];
            busyIndicator.hidden = YES;
        }];
        return;
    }
    
    [_localFileList removeAllObjects];
    if (!_files) {
        return;
    }

    for (NSString* name in _files.fileWrappers) {
        NSFileWrapper* file = _files.fileWrappers[name];
        if (file && file.regularFile) {
            [_localFileList addObject:@{ @"name" : name, @"size" : @(_files.fileWrappers[name].regularFileContents.length) }];
        }
    }
    
    [_localFileList sortUsingComparator:^NSComparisonResult(NSDictionary* a, NSDictionary* b) {
        return [a[@"name"] compare:b[@"name"]];
    }];
    [fileListView reloadData];
}

- (IBAction)reloadDevices:(id)sender
{
}

- (IBAction)fileSelected:(id)sender
{
    NSIndexSet* indexes = ((NSTableView*) sender).selectedRowIndexes;
    if (indexes.count != 1) {
        // clear text editor
        [_document setSource:@""];
    } else {
        NSString* name = self.currentFileList[fileListView.selectedRow][@"name"];

        if (!self.isFileSourceLocal) {
            [busyIndicator setHidden:NO];
            [busyIndicator startAnimation:nil];
            
            [_device selectFile:name withBlock:^(NSString* content) {
                [_document setSource:content];
                [busyIndicator stopAnimation:nil];
                busyIndicator.hidden = YES;
            }];
            return;
        }

        [_document setSource:[[NSString alloc] initWithData:
            _files.fileWrappers[name].regularFileContents encoding:NSUTF8StringEncoding]];
    }
}

- (void)addFiles
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setPrompt:@"Add"];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            BOOL __block cancelAdd = NO;
            BOOL __block skipAdd = NO;
            
            for (NSURL* url in [panel URLs]) {
                NSString* toName = url.lastPathComponent;
                if ([self fileListContains:toName]) {
                    NSAlert *alert = [[NSAlert alloc] init];
                    [alert addButtonWithTitle:@"Cancel"];
                    [alert addButtonWithTitle:@"Yes"];
                    [alert addButtonWithTitle:@"Skip"];
                    [alert setMessageText:[NSString stringWithFormat:@"%@ exists, overwrite?", toName]];
                    [alert setInformativeText:@"This operation cannot be undone."];
                    [alert setAlertStyle:NSWarningAlertStyle];

                    [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
                        if (returnCode == NSAlertFirstButtonReturn) {
                            // Cancel
                            cancelAdd = YES;
                            return;
                        }
                        if (returnCode == NSAlertThirdButtonReturn) {
                            // Skip
                            skipAdd = YES;
                            return;
                        }
                    }];
                    
                    if (cancelAdd) {
                        return;
                    }
                    
                    if (skipAdd) {
                        skipAdd = NO;
                        continue;
                    }
                }

                NSFileWrapper* file  = [[NSFileWrapper alloc] initRegularFileWithContents:[NSData dataWithContentsOfURL:url]];
                file.preferredFilename = toName;

                if (self.isFileSourceLocal) {
                    NSFileWrapper* files = self.filesFileWrapper;
                    if (files && files.fileWrappers[toName]) {
                        [files removeFileWrapper:files.fileWrappers[toName]];
                    }
                    [files addFileWrapper:file];
                    [_document markDirty];
                } else {
                    [_device addFile:file];
                }
            }
                
            [self reloadFiles];
        }
    }];
}

- (void)removeFiles
{
    NSIndexSet* indexes = fileListView.selectedRowIndexes;
    if (!indexes.count) {
        return;
    }
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"Cancel"];
    [alert addButtonWithTitle:@"OK"];
    if (indexes.count == 1) {
        [alert setMessageText:@"Delete this file?"];
    } else {
        [alert setMessageText:@"Delete these files?"];
    }
    [alert setInformativeText:@"This operation cannot be undone."];
    [alert setAlertStyle:NSWarningAlertStyle];
    [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) {
            return;
        }

        NSUInteger i = 0;
        for (NSDictionary* entry in self.currentFileList) {
            if ([indexes containsIndex:i++]) {
                if (self.isFileSourceLocal) {
                    NSFileWrapper* files = self.filesFileWrapper;
                    NSFileWrapper* d = files.fileWrappers[[entry objectForKey:@"name"]];
                    if (d) {
                        [_document markDirty];
                        [files removeFileWrapper:d];
                    }
                } else {
                    [_device removeFile:[entry objectForKey:@"name"]];
                }
            }
        }
    
        [fileListView deselectAll:self];
        [self reloadFiles];
    }];
}

- (NSString*)trimTrailingDot:(NSString*)s
{
    if ([s characterAtIndex:s.length - 1] == '.') {
        return [s substringToIndex:s.length - 1];
    }
    return s;
}

- (NSNetService*)serviceFromDevice:(NSDictionary*)device
{
    return (NSNetService*) (device[@"service"]);
}

- (IBAction)changeFileSource:(id)sender
{
    [_device setDevice:[(NSPopUpButton*)sender titleOfSelectedItem]];
    [_document setSource:@""];
    [fileListView deselectAll:self];
    [self reloadFiles];
}

- (IBAction)reloadDeviceList:(id)sender {
}

- (void)renameDevice
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"Cancel"];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:@"Rename device:"];
    [alert setAccessoryView:renameDeviceTextField];
    [alert setAlertStyle:NSInformationalAlertStyle];
    [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) {
            return;
        }
        NSString* name = renameDeviceTextField.stringValue;
        NSString* errorString;
        int returnType = validateBonjourName(name.UTF8String);
        if (returnType == NameValidationBadLength) {
            errorString = @"device name must be between 1 and 31 characters";
        } else if (returnType == NameValidationInvalidChar) {
            errorString = @"device name must only contain numbers, lowercase letters and hyphen";
        }
        if (errorString) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"OK"];
            [alert setMessageText:errorString];
            [alert setAlertStyle:NSWarningAlertStyle];
            [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
            }];
            return;
        }
        
        [_device renameDevice:name];
    }];
}

- (void)upload
{
}

// fileListView dataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [self.currentFileList count];
}

// TableView delegagte
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    if (self.currentFileList.count <= rowIndex) {
        return nil;
    }
    return [[self.currentFileList objectAtIndex:rowIndex] objectForKey:aTableColumn.identifier];
}

- (void)tableView:(NSTableView *)aTableView
   setObjectValue:(id)anObject
   forTableColumn:(NSTableColumn *)aTableColumn
              row:(NSInteger)rowIndex
{
    return;
}

// Device methods
- (void)clearDeviceList
{
}

- (void)addDevice:(NSString*)name
{
    [fileSourceButton addItemWithTitle:name];
    [fileSourceButton setNeedsDisplay];
}

@end
