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
#import "FileBrowser.h"

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
    __weak IBOutlet NSView *filesContainer;
    
    __weak IBOutlet NSToolbarItem *runButton;
    __weak IBOutlet NSToolbarItem *buildButton;
    __weak IBOutlet NSToolbarItem *pauseButton;
    __weak IBOutlet NSToolbarItem *stopButton;
    __weak IBOutlet NSToolbarItem *uploadButton;
    __weak IBOutlet NSToolbarItem *simulateButton;
    __weak IBOutlet NSToolbarItem *addFileButton;
    __weak IBOutlet NSToolbarItem *removeFileButton;
    __weak IBOutlet NSToolbarItem *reloadFilesButton;    
    
    NSString* _source;
    NSFont* _font;
    NSNetServiceBrowser* _netServiceBrowser;
    NSNetService* _netService;
        
    Simulator* _simulator;
    FileBrowser* _fileBrowser;
    
    NSFileWrapper* _package;
}

@end

@implementation Document

- (instancetype)init {
    self = [super init];
    if (self) {
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
        
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

    _fileBrowser = [[FileBrowser alloc] initWithDocument:self];
    [filesContainer addSubview:_fileBrowser.view];
    superFrame = filesContainer.frame;
    [_fileBrowser.view setFrameSize:superFrame.size];   
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
        return _fileBrowser.isFileSourceLocal;
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

- (void)markDirty
{
    [self updateChangeCount:NSChangeReadOtherContents];
}

- (void)setSource:(NSString*)source
{
    _source = source;
    if (sourceEditor) {
        [sourceEditor setString:_source];
    }
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

- (NSString *)windowNibName {
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"Document";
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError {
    return [sourceEditor.string dataUsingEncoding:NSUTF8StringEncoding];
}

- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError {
    [self setSource:[NSString stringWithUTF8String:(const char*)[data bytes]]];
    return YES;
}

- (BOOL)readFromFileWrapper:(NSFileWrapper *)fileWrapper
                     ofType:(NSString *)typeName 
                      error:(NSError * _Nullable *)outError
{
    NSAssert(_package == nil, @"Package should not exist when readFromFileWrapper is called");
    _package = fileWrapper;
    NSFileWrapper*files = (_package.fileWrappers && _package.fileWrappers[@"Contents"]) ?
                                _package.fileWrappers[@"Contents"].fileWrappers[@"Files"] : 
                                nil;
    [_fileBrowser setFiles: files];
    return YES;
}

- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError **)outError
{
    if (!_package) {
        [self markDirty];
        NSFileWrapper* pkgInfo = [[NSFileWrapper alloc] initRegularFileWithContents: [NSData dataWithBytes:(void*)"????????" length:8]];
        NSFileWrapper *contentsFileWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"PkgInfo" : pkgInfo, @"Files" : _fileBrowser.files }];
        _package = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Contents" : contentsFileWrapper }];
    }
    return _package;
}

- (void)clearOutput:(OutputType)output
{
    [((output == CTBuild) ? buildOutput : consoleOutput) setString: @""];
}

- (IBAction)addFile:(id)sender {
    [_fileBrowser addFiles];
}

- (IBAction)removeFile:(id)sender {
    [_fileBrowser removeFiles];
}

- (IBAction)reloadFiles:(id)sender {
    [_fileBrowser reloadFiles];
}

- (IBAction)upload:(id)sender {
    [_fileBrowser upload];
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
