//
//  FileBrowser.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "FileBrowser.h"

#import "Document.h"
#import "Device.h"

@interface FileBrowser ()
{
    __weak IBOutlet NSProgressIndicator *busyIndicator;
    __weak IBOutlet NSTableView *fileListView;
    __weak IBOutlet NSPopUpButton *fileSourceButton;

    Document* _document;
    
    FileList _currentFileList;

    NSTextField* renameDeviceTextField;
}

@end

@implementation FileBrowser

- (instancetype)initWithDocument:(Document*) document {
    self = [super init];
    if (self) {
        _document = document;
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

    [fileListView registerForDraggedTypes: [NSArray arrayWithObjects: NSFilenamesPboardType, @"public.jpeg", NSURLPboardType, NSStringPboardType, nil]];
    [fileListView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
}

- (BOOL)isFileSourceLocal
{
    return [fileSourceButton.selectedItem.title isEqualToString:@"Local Files"];
}

- (BOOL)fileListContains:(NSString*)name
{
    for (NSDictionary* entry in _currentFileList) {
        if ([entry[@"name"] isEqualToString:name]) {
            return YES;
        }
    }
    return NO;
}

- (void)reloadFilesForDevice:(Device*)device;
{
    [busyIndicator setHidden:NO];
    [busyIndicator startAnimation:nil];
    [device reloadFilesWithBlock:^(FileList fileList) {
        _currentFileList = fileList;
        [fileListView reloadData];
        [busyIndicator stopAnimation:nil];
        busyIndicator.hidden = YES;
    }];
}

- (IBAction)reloadDevices:(id)sender
{
}

- (IBAction)fileSelected:(id)sender
{
    NSInteger index = -1;
    NSIndexSet* indexes = ((NSTableView*) sender).selectedRowIndexes;
    if (indexes.count == 1) {
        index = fileListView.selectedRow;
    }
    [_document selectFile:index];
}

- (void)selectFile:(NSString*)name
{
    NSInteger index = 0;
    for (NSDictionary* entry in _currentFileList) {
        if ([entry[@"name"] isEqualToString:name]) {
            [_document selectFile:index];
            return;
        }
        index++;
    }
}

- (NSInteger)selectedFileCount
{
    NSIndexSet* indexes = fileListView.selectedRowIndexes;
    return indexes.count;
}

- (NSString*)selectedFileName
{
    if ([self selectedFileCount] != 1) {
        return nil;
    }

    NSIndexSet* indexes = fileListView.selectedRowIndexes;
    NSUInteger i = 0;
    for (NSDictionary* entry in _currentFileList) {
        if ([indexes containsIndex:i++]) {
            return entry[@"name"];
        }
    }
    return nil;
}

- (void)newFile
{
    NSFileWrapper* file  = [[NSFileWrapper alloc] initRegularFileWithContents:[NSData data]];
    NSString* name = @"Untitled";
    uint32_t n = 1;
    while ([self fileListContains:name]) {
        name = [NSString stringWithFormat:@"Untitled%d", n++];
    }

    file.preferredFilename = name;
    file.filename = name;
    [_document addFile:file];
    [fileListView deselectAll:self];
    [_document reloadFiles];
}

- (void)addFiles
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = YES;
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
                [_document addFile:file];
            }
                
            [fileListView deselectAll:self];
            [_document reloadFiles];
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
        for (NSDictionary* entry in _currentFileList) {
            if ([indexes containsIndex:i++]) {
                [_document removeFile:entry[@"name"]];
            }
        }
    
        [fileListView deselectAll:self];
        [_document reloadFiles];
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
    [fileListView deselectAll:self];
    [_document setDevice:[(NSPopUpButton*)sender titleOfSelectedItem]];
}

- (IBAction)reloadDeviceList:(id)sender {
}

- (NSString*)getNewDeviceName
{
    NSAlert *alert = [[NSAlert alloc] init];
    NSString* __block name = nil;
    
    [alert addButtonWithTitle:@"Cancel"];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:@"Rename device:"];
    [alert setAccessoryView:renameDeviceTextField];
    [alert setAlertStyle:NSInformationalAlertStyle];
    [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) {
            return;
        }
        name = renameDeviceTextField.stringValue;
        NSString* errorString;
        int returnType = validateBonjourName(name.UTF8String);
        if (returnType == NameValidationBadLength) {
            errorString = @"device name must be between 1 and 31 characters";
        } else if (returnType == NameValidationInvalidChar) {
            errorString = @"device name must only contain numbers, lowercase letters and hyphen";
        }
        if (errorString) {
            name = nil;
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"OK"];
            [alert setMessageText:errorString];
            [alert setAlertStyle:NSWarningAlertStyle];
            [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) { }];
        }
    }];
    
    return name;
}

- (void)addDevice:(NSString*)name
{
    [fileSourceButton addItemWithTitle:name];
    [fileSourceButton setNeedsDisplay];
}

// fileListView dataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [_currentFileList count];
}

// TableView delegate
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    if (_currentFileList.count <= rowIndex) {
        return nil;
    }
    return [[_currentFileList objectAtIndex:rowIndex] objectForKey:aTableColumn.identifier];
}

- (void)tableView:(NSTableView *)aTableView
   setObjectValue:(id)anObject
   forTableColumn:(NSTableColumn *)aTableColumn
              row:(NSInteger)rowIndex
{
    // The name cell is the only thing modifiable. Assume it's that.
    // Name in cell in rowIndex has been changed to the string in anObject
    NSString* oldName = [_currentFileList objectAtIndex:rowIndex][@"name"];
    if (!oldName || ![anObject isKindOfClass:[NSString class]]) {
        return;
    }
    
    NSString* newName = (NSString*) anObject;
    
    if ([self fileListContains:newName]) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Yes"];
        [alert setMessageText:[NSString stringWithFormat:@"%@ exists, overwrite?", newName]];
        [alert setInformativeText:@"This operation cannot be undone."];
        [alert setAlertStyle:NSWarningAlertStyle];

        if ([alert runModal] == NSAlertFirstButtonReturn) {
            return;
        }
        [_document removeFile:newName];
    }
        
    [_document renameFileFrom:oldName to:newName];
    [fileListView deselectAll:self];
    [_document reloadFiles];
}

- (BOOL) tableView: (NSTableView *) view
         writeRows: (NSArray *) rows
         toPasteboard: (NSPasteboard *) pboard
{
    [pboard declareTypes:[NSArray arrayWithObjects:NSFilesPromisePboardType, nil] owner:self];

    // the pasteboard must know the type of files being promised
    NSMutableArray *filenameExtensions = [NSMutableArray array];

    for (NSNumber* index in rows) {
        NSString *filename = [_currentFileList objectAtIndex:[index integerValue]][@"name"];
        NSString *filenameExtension = [filename pathExtension];

        if (![filenameExtension isEqualToString:@""])
        {
            [filenameExtensions addObject:filenameExtension];
        }
    } 

    [pboard setPropertyList:filenameExtensions forType:NSFilesPromisePboardType];

    return YES;
}

- (NSDragOperation) tableView: (NSTableView *) view
                    validateDrop: (id <NSDraggingInfo>) info
                    proposedRow: (int) row
                    proposedDropOperation: (NSTableViewDropOperation) operation
{
    return ([info draggingSource] != fileListView) ? NSDragOperationMove : NSDragOperationNone;
}

- (BOOL) tableView: (NSTableView *) view
         acceptDrop: (id <NSDraggingInfo>) info
         row: (int) row
         dropOperation: (NSTableViewDropOperation) operation
{
    NSPasteboard *pboard = [info draggingPasteboard];
    NSDictionary* fileList = [pboard propertyListForType:NSFilenamesPboardType];
    for (NSString* filename in fileList) {
        NSURL* url = [NSURL fileURLWithPath:filename];
        NSFileWrapper* file  = [[NSFileWrapper alloc] initRegularFileWithContents:[NSData dataWithContentsOfURL:url]];
        file.preferredFilename = [url lastPathComponent];
        [_document addFile:file];
    }
                
    [fileListView deselectAll:self];
    [_document reloadFiles];
    return YES;
}

- (NSArray*)tableView:(NSTableView*)aTableView namesOfPromisedFilesDroppedAtDestination:(NSURL*)dropDestination forDraggedRowsWithIndexes:(NSIndexSet*)indexSet 
{
    NSMutableArray *draggedFilenames = [NSMutableArray array];
    NSArray* fileList = [_currentFileList objectsAtIndexes:indexSet];
    
    for (NSDictionary* entry in fileList) {
        //NSURL *url = [_document saveToTempFile:entry[@"name"]];
        if ([_document saveFile:entry[@"name"] withURLBase:dropDestination]) {
            [draggedFilenames addObject:entry[@"name"]];
        }
    }

    return draggedFilenames;
}

// This method is called when the drag starts and can be used to create a custom drag image
-(void)tableView:(NSTableView *)tableView
 draggingSession: (NSDraggingSession *)session
willBeginAtPoint:(NSPoint)screenPoint
   forRowIndexes:(NSIndexSet *)rowIndexes {
   NSLog(@"*****");
}

@end
