//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "NSTextView+JSDExtensions.h"

#import "MacFS.h"
#import "Simulator.h"

#import <iostream>
#import <stdarg.h>
#import <sstream>
#import <thread>
#import <chrono>
#import <cstdio>

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    __weak IBOutlet NSTabView *outputView;
    __weak IBOutlet NSTableView *fileListView;
    __weak IBOutlet NSView *simContainer;
    
    __weak IBOutlet NSToolbarItem *runButton;
    __weak IBOutlet NSToolbarItem *buildButton;
    __weak IBOutlet NSToolbarItem *pauseButton;
    __weak IBOutlet NSToolbarItem *stopButton;
    __weak IBOutlet NSToolbarItem *uploadButton;
    __weak IBOutlet NSToolbarItem *simulateButton;
    __weak IBOutlet NSToolbarItem *addFileButton;
    __weak IBOutlet NSToolbarItem *removeFileButton;
    __weak IBOutlet NSToolbarItem *reloadFilesButton;    
    __weak IBOutlet NSPopUpButton *fileSourceButton;
    
    NSString* _source;
    NSFont* _font;
    NSMutableArray* _fileList;
    NSNetServiceBrowser* _netServiceBrowser;
    NSNetService* _netService;
        
    Simulator* _simulator;
    
    NSFileWrapper* _package;
    NSFileWrapper* _files;
}

@end

@implementation Document

- (instancetype)init {
    self = [super init];
    if (self) {
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
        
        _fileList = [[NSMutableArray alloc] init];

        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
    }
    return self;
}

- (void)awakeFromNib
{
    [sourceEditor setFont:_font];
    [consoleOutput setFont:_font];
    [buildOutput setFont:_font];
    sourceEditor.ShowsLineNumbers = YES;
    sourceEditor.automaticQuoteSubstitutionEnabled = NO;
    [[sourceEditor textStorage] setDelegate:(id) self];
    if (_source) {
        [sourceEditor setString:_source];
    }
    _simulator = [[Simulator alloc] initWithDocument:self];
    [simContainer addSubview:_simulator.view];
    
    NSRect superFrame = simContainer.frame;
    [_simulator.view setFrameSize:superFrame.size];
}

-(BOOL) validateToolbarItem:(NSToolbarItem*) item
{
    if (item == buildButton || item == simulateButton) {
        return YES;
    }
    if (item == runButton) {
        return [_simulator canRun];
    }
    if (item == stopButton) {
        return [_simulator canStop];
    }
    if (item == addFileButton || item == removeFileButton || item == reloadFilesButton) {
        return [fileSourceButton.selectedItem.title isEqualToString:@"Local Files"];
    }
    return NO;
}

+ (BOOL)autosavesInPlace {
    return YES;
}

//
// Simulator Interface
//
- (IBAction)build:(id)sender
{
    [_simulator build:[sourceEditor.string UTF8String] withName:[self displayName]];
}

- (IBAction)run:(id)sender
{
    [_simulator run];
}

- (IBAction)pause:(id)sender
{
    [_simulator pause];
}

- (IBAction)stop:(id)sender
{
    [_simulator stop];
}

- (IBAction)simulate:(id)sender
{
    [_simulator simulate];
}

- (void)outputMessage:(NSString*) message to:(OutputType) output
{
    if (output == CTBuild) {
        [outputView selectTabViewItemAtIndex:1];
    } else {
        [outputView selectTabViewItemAtIndex:0];
    }
    NSTextView* view = (output == CTBuild) ? buildOutput : consoleOutput;
    NSString* string = [NSString stringWithFormat: @"%@%@", view.string, message];
    [view setString:string];
    [view scrollRangeToVisible:NSMakeRange([[view string] length], 0)];
    [view setNeedsDisplay:YES];
}

- (IBAction)importBinary:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setAllowedFileTypes:@[@"m8rp"]];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [[panel URLs] objectAtIndex:0];
            [_simulator importBinary:[url fileSystemRepresentation]];
        }
    }];
}

- (IBAction)exportBinary:(id)sender
{
    NSString *filename = [[self.fileURL absoluteString] lastPathComponent];
    NSString* newName = [[filename stringByDeletingPathExtension]
                                   stringByAppendingPathExtension:@"m8rp"];
    
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel setNameFieldStringValue:newName];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [panel URL];
            [_simulator exportBinary:[url fileSystemRepresentation]];
        }
    }];
}

//
// Text Content Interface
//
- (void)textStorageDidProcessEditing:(NSNotification *)notification {
    NSTextStorage *textStorage = notification.object;
    NSString *string = textStorage.string;
    NSUInteger n = string.length;
    [textStorage removeAttribute:NSForegroundColorAttributeName range:NSMakeRange(0, n)];
    for (NSUInteger i = 0; i < n; i++) {
        unichar c = [string characterAtIndex:i];
        if (c == '\\') {
            [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor lightGrayColor] range:NSMakeRange(i, 1)];
            i++;
        } else if (c == '$') {
            NSUInteger l = ((i < n - 1) && isdigit([string characterAtIndex:i+1])) ? 2 : 1;
            [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor redColor] range:NSMakeRange(i, l)];
            i++;
        }
    }
}

//
// File system interface
//
- (NSFileWrapper*)filesFileWrapper
{
    if (!_files) {
        [self updateChangeCount:NSChangeReadOtherContents];
        _files = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ }];
    }
    return _files;
}

- (IBAction)fileSelected:(id)sender
{
}

static void addFileToList(NSMutableArray* list, NSString* name, uint32_t size)
{
    [list addObject:@{ @"name" : name, @"size" : [NSNumber numberWithInt:size] }];
}

- (void)reloadFiles
{
    [_fileList removeAllObjects];
    if (!_files) {
        return;
    }
    
    for (NSString* name in _files.fileWrappers) {
        NSFileWrapper* file = _files.fileWrappers[name];
        if (file && file.regularFile) {
            addFileToList(_fileList, name, (uint32_t) _files.fileWrappers[name].regularFileContents.length);
        }
    }
    [fileListView reloadData];
}

- (NSString *)windowNibName {
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"Document";
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError {
    return [sourceEditor.string dataUsingEncoding:NSUTF8StringEncoding];
}

- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError {
    _source = [NSString stringWithUTF8String:(const char*)[data bytes]];
    if (sourceEditor) {
        [sourceEditor setString:_source];
    }
    return YES;
}

- (BOOL)readFromFileWrapper:(NSFileWrapper *)fileWrapper
                     ofType:(NSString *)typeName 
                      error:(NSError * _Nullable *)outError
{
    NSAssert(_package == nil, @"Package should not exist when readFromFileWrapper is called");
    _package = fileWrapper;
    [self reloadFiles];
    return YES;
}

- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError **)outError
{
    if (!_package) {
        [self updateChangeCount:NSChangeReadOtherContents];
        NSFileWrapper* pkgInfo = [[NSFileWrapper alloc] initRegularFileWithContents: [NSData dataWithBytes:(void*)"????????" length:8]];
        NSFileWrapper *contentsFileWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"PkgInfo" : pkgInfo, @"Files" : self.filesFileWrapper }];
        _package = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Contents" : contentsFileWrapper }];
    }
    return _package;
}

- (void)clearOutput:(OutputType)output
{
    [((output == CTBuild) ? buildOutput : consoleOutput) setString: @""];
}

- (IBAction)addFile:(id)sender {
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
            
            [self updateChangeCount:NSChangeReadOtherContents];
            NSFileWrapper* file  = [[NSFileWrapper alloc] initRegularFileWithContents:[NSData dataWithContentsOfURL:url]];
            file.preferredFilename = toName;
            [files addFileWrapper:file];
            [self reloadFiles];
        }
    }];
}

- (IBAction)changeFileSource:(id)sender {
}

- (IBAction)removeFile:(id)sender {
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
                [self updateChangeCount:NSChangeReadOtherContents];
                [files removeFileWrapper:d];
            }
        }
    }
    [fileListView deselectAll:self];
    [self reloadFiles];
}

- (IBAction)reloadFiles:(id)sender {
    [self reloadFiles];
}

- (IBAction)upload:(id)sender {
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

// NSNetServiceBrowser delegate
- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *)netServiceBrowser
{
    NSLog(@"*** netServiceBrowserWillSearch\n");
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser
           didFindService:(NSNetService *)netService
               moreComing:(BOOL)moreServicesComing
{
    _netService = netService;
    [netService setDelegate:(id) self];
    NSLog(@"*** Found service: %@\n", netService);
    [netService resolveWithTimeout:10];
}

-(void)netServiceBrowser:(NSNetServiceBrowser *)browser didNotSearch:(NSDictionary<NSString *,NSNumber *> *)errorDict {
    NSLog(@"not search ");
}

- (void)netServiceWillResolve:(NSNetService *)sender
{
    NSLog(@"********* Will resolve\n");
}

- (void)netServiceDidResolveAddress:(NSNetService *)sender
{
    NSLog(@"********* Did resolve\n");
}

- (void)netService:(NSNetService *)sender 
     didNotResolve:(NSDictionary<NSString *,NSNumber *> *)errorDict
{
    NSLog(@"********* Did not resolve: %@\n", errorDict);
}

@end
