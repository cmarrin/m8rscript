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
    
    NSURL* _folder;
    
    Simulator* _simulator;
}

@end

@implementation Document

- (instancetype)init {
    self = [super init];
    if (self) {
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
        
        setFileSystemPath();
        _fileList = [[NSMutableArray alloc] init];
        [self reloadFiles];

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
    //[_simulator.view setFrameOrigin:CGPointMake(0, 0)];
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
static inline void setFileSystemPath()
{
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSArray* possibleURLs = [fileManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
    NSURL* appSupportDir;
    
    if ([possibleURLs count] >= 1) {
        appSupportDir = [possibleURLs objectAtIndex:0];
    }

    if (!appSupportDir) {
        return;
    }
    
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];

    NSString* path = [appSupportDir.path stringByAppendingFormat:@"/%@/Files/", bundleId];
 
    NSError *error = nil;
    if(![fileManager createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&error]) {
        NSLog(@"Failed to create directory \"%@\". Error: %@", path, error);
    }
    
    m8r::MacFS::setFileSystemPath([path UTF8String]);
}

static void addFileToList(NSMutableArray* list, const char* name, uint32_t size)
{
    [list addObject:@{ @"name" : [NSString stringWithUTF8String:name], @"size" : [NSNumber numberWithInt:size] }];
}

- (void)reloadFiles
{
    [_fileList removeAllObjects];
    m8r::DirectoryEntry* entry = m8r::FS::sharedFS()->directory();
    while (entry && entry->valid()) {
        addFileToList(_fileList, entry->name(), entry->size());
        entry->next();
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
            
            FILE* fromFile = fopen([url fileSystemRepresentation], "r");
            NSString* toName = url.lastPathComponent;
            m8r::File* toFile = m8r::FS::sharedFS()->open([toName UTF8String], "w");
            while(!feof(fromFile)) {
                int c = fgetc(fromFile);
                if (c < 0) {
                    break;
                }
                toFile->write(c);
            }
            fclose(fromFile);
            delete toFile;
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
    [alert beginSheetModalForWindow:fileListView.window completionHandler:^(NSInteger result){
        if (result != NSAlertFirstButtonReturn) {
            return;
        }

        NSUInteger i = 0;
        for (NSDictionary* entry in _fileList) {
            if ([indexes containsIndex:i++]) {
                m8r::FS::sharedFS()->remove([[entry objectForKey:@"name"] UTF8String]);
            }
        }
        [fileListView deselectAll:self];
        [self reloadFiles];
    }];
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
