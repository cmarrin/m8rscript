//
//  FileBrowser.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "FileBrowser.h"

#import "Document.h"

@interface FileBrowser ()
{
    __weak IBOutlet NSTableView *fileListView;
    __weak IBOutlet NSPopUpButton *fileSourceButton;

    Document* _document;

    NSMutableArray* _fileList;
    NSFileWrapper* _files;
}

@end

@implementation FileBrowser

- (instancetype)initWithDocument:(Document*) document {
    self = [super init];
    if (self) {
        _document = document;

        _fileList = [[NSMutableArray alloc] init];
     }
    return self;
}

- (void)dealloc
{
}

- (BOOL)isFileSourceLocal
{
    return [fileSourceButton.selectedItem.title isEqualToString:@"Local Files"];
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

- (IBAction)fileSelected:(id)sender
{
    NSIndexSet* indexes = ((NSTableView*) sender).selectedRowIndexes;
    if (indexes.count != 1) {
        // clear text editor
        [_document setSource:nil];
    } else {
        NSString* name = _fileList[fileListView.selectedRow][@"name"];
        [_document setSource:[[NSString alloc] initWithData:
            _files.fileWrappers[name].regularFileContents encoding:NSUTF8StringEncoding]];
    }
}

- (void)reloadFiles
{
    [_fileList removeAllObjects];
    if (!_files) {
        return;
    }
    
    NSArray* sortedKeys = [_files.fileWrappers.allKeys sortedArrayUsingSelector:@selector(compare:)];
    
    for (NSString* name in sortedKeys) {
        NSFileWrapper* file = _files.fileWrappers[name];
        if (file && file.regularFile) {
            [_fileList addObject:@{ @"name" : name, @"size" : @(_files.fileWrappers[name].regularFileContents.length) }];
        }
    }
    
    [fileListView reloadData];
}

- (void)addFiles
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setPrompt:@"Add"];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [[panel URLs] objectAtIndex:0];
            NSString* toName = url.lastPathComponent;
            
            NSFileWrapper* files = self.filesFileWrapper;
            if (files && files.fileWrappers[toName]) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert addButtonWithTitle:@"Cancel"];
                [alert addButtonWithTitle:@"OK"];
                [alert setMessageText:[NSString stringWithFormat:@"%@ exists, overwrite?", toName]];
                [alert setInformativeText:@"This operation cannot be undone."];
                [alert setAlertStyle:NSWarningAlertStyle];
                if ([alert runModal] == NSModalResponseCancel) {
                    return;
                }
                
                [files removeFileWrapper:files.fileWrappers[toName]];
            }
            
            [_document markDirty];
            NSFileWrapper* file  = [[NSFileWrapper alloc] initRegularFileWithContents:[NSData dataWithContentsOfURL:url]];
            file.preferredFilename = toName;
            [files addFileWrapper:file];
            [self reloadFiles];
        }
    }];
}

- (IBAction)changeFileSource:(id)sender {
}

- (void)removeFiles
{
    NSIndexSet* indexes = fileListView.selectedRowIndexes;
    if (!indexes.count) {
        return;
    }
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    if (indexes.count == 1) {
        [alert setMessageText:@"Delete this file?"];
    } else {
        [alert setMessageText:@"Delete these files?"];
    }
    [alert setInformativeText:@"This operation cannot be undone."];
    [alert setAlertStyle:NSWarningAlertStyle];
    if ([alert runModal] == NSModalResponseCancel) {
        return;
    }

    NSUInteger i = 0;
    NSFileWrapper* files = self.filesFileWrapper;
    if (!files) {
        return;
    }
    
    for (NSDictionary* entry in _fileList) {
        if ([indexes containsIndex:i++]) {
            NSFileWrapper* d = files.fileWrappers[[entry objectForKey:@"name"]];
            if (d) {
                [_document markDirty];
                [files removeFileWrapper:d];
            }
        }
    }
    [fileListView deselectAll:self];
    [self reloadFiles];
}

- (void)upload
{
}

// fileListView dataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [_fileList count];
}

// TableView delegagte
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    return [[_fileList objectAtIndex:rowIndex] objectForKey:aTableColumn.identifier];
}

- (void)tableView:(NSTableView *)aTableView
   setObjectValue:(id)anObject
   forTableColumn:(NSTableColumn *)aTableColumn
              row:(NSInteger)rowIndex
{
    return;
}

@end
