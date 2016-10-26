//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "NSTextView+JSDExtensions.h"

#import "Device.h"
#import "SimulationView.h"
#import "FileBrowser.h"

#import <stdarg.h>

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    __weak IBOutlet NSTabView *outputView;
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
    __weak IBOutlet NSToolbarItem *SaveBinaryButton;
    
    NSString* _source;
    NSFont* _font;
    NSString* _sourceFilename;
    
    Device* _device;
    SimulationView* _simulationView;
    FileBrowser* _fileBrowser;
    
    NSFileWrapper* _package;
    
}

@end

@implementation Document

- (instancetype)init {
    self = [super init];
    if (self) {
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
        [self clearContents];
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
    [sourceEditor setString:_source];
    
    _device = [[Device alloc]init];
    _device.delegate = self;
    
    _simulationView = [[SimulationView alloc] init];
    [simContainer addSubview:_simulationView.view];
    NSRect superFrame = simContainer.frame;
    [_simulationView.view setFrameSize:superFrame.size];

    _fileBrowser = [[FileBrowser alloc] initWithDocument:self];
    [filesContainer addSubview:_fileBrowser.view];
    superFrame = filesContainer.frame;
    [_fileBrowser.view setFrameSize:superFrame.size];
    
    if (_package) {
        [self setFiles];
    }
}

-(BOOL) validateToolbarItem:(NSToolbarItem*) item
{
    if (item == buildButton) {
        return [_source length] && [[_sourceFilename pathExtension] isEqualToString:@"m8r"];
    }
    if (item == simulateButton) {
        return [_device canSimulate];
    }
    if (item == runButton) {
        return [_device canRun];
    }
    if (item == stopButton) {
        return [_device canStop];
    }
    if (item == addFileButton) {
        return YES;
    }
    if (item == removeFileButton) {
        return [_fileBrowser selectedFileCount];
    }
    if (item == uploadButton) {
        return [_device canUpload];
    }
    if (item == SaveBinaryButton) {
        return [_device canSaveBinary];
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
    [self clearOutput:CTBuild];
    [_device build:[sourceEditor.string UTF8String] withName:_sourceFilename];
}

- (IBAction)run:(id)sender
{
    [self clearOutput:CTConsole];
    [_device run];
}

- (IBAction)pause:(id)sender
{
    [_device pause];
}

- (IBAction)stop:(id)sender
{
    [_device stop];
}

- (IBAction)simulate:(id)sender
{
    [_device simulate];
}

- (IBAction)saveBinary:(id)sender {
    [_device saveBinary:_sourceFilename];
    [self reloadFiles];
}

- (void)outputMessage:(NSString*) message toBuild:(BOOL) build
{
    if (build) {
        [outputView selectTabViewItemAtIndex:1];
    } else {
        [outputView selectTabViewItemAtIndex:0];
    }
    NSTextView* view = build ? buildOutput : consoleOutput;
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
            [_device importBinary:[url fileSystemRepresentation]];
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
            [_device exportBinary:[url fileSystemRepresentation]];
        }
    }];
}

- (IBAction)addFiles:(id)sender
{
    [_fileBrowser addFiles];
}

- (IBAction)removeFiles:(id)sender
{
    [_fileBrowser removeFiles];
}

- (void)markDirty
{
    [self updateChangeCount:NSChangeDone];
}

- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode
{
    [_simulationView updateGPIOState:state withMode:mode];
}

- (void)addDevice:(NSString*)name
{
    [_fileBrowser addDevice:name];
}

- (void)clearContents
{
    _source = @"";
    _sourceFilename = @"";
    if (sourceEditor) {
        [sourceEditor setString:_source];
    }
    [_device clearContents];
}

- (void)setContents:(NSData*)contents withName:(NSString*)name;
{
    [self clearContents];
    
    NSString* source = [[NSString alloc] initWithData:contents encoding:NSUTF8StringEncoding];
    if (source) {
        _source = source;
        _sourceFilename = name;
        if (sourceEditor) {
            [sourceEditor setString:_source];
        }
        return;
    }
    
    NSImage* image = [[NSImage alloc]initWithData:contents];
    if (image) {
        [self setImage:image];
    }
}

- (void)setImage:(NSImage*)image
{
    NSTextAttachmentCell *attachmentCell = [[NSTextAttachmentCell alloc] initImageCell:image];
    NSTextAttachment *attachment = [[NSTextAttachment alloc] init];
    [attachment setAttachmentCell: attachmentCell ];
    NSAttributedString *attributedString = [NSAttributedString  attributedStringWithAttachment: attachment];
    [[sourceEditor textStorage] setAttributedString:attributedString];
}

- (void)setFiles
{
    if (!_fileBrowser) {
        return;
    }
    
    NSFileWrapper* files = (_package.fileWrappers && _package.fileWrappers[@"Contents"]) ?
                                _package.fileWrappers[@"Contents"].fileWrappers[@"Files"] : 
                                nil;
    [_device setFiles: files];
    [self reloadFiles];
}

- (void)selectFile:(NSInteger)index
{
    [self clearContents];
    if (index < 0) {
        return;
    }
    
    [_device selectFile:index];
}

- (void)addFile:(NSFileWrapper*)file
{
    [self clearContents];
    [_device addFile:file];
}

- (void)removeFile:(NSString*)name
{
    [_device removeFile:name];
}

- (void)setDevice:(NSString*)name
{
    [_device setDevice:name];
    [self clearContents];
    [self reloadFiles];
}

//
// Text Content Interface
//
- (void)textStorageDidProcessEditing:(NSNotification *)notification
{
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

- (BOOL)readFromFileWrapper:(NSFileWrapper *)fileWrapper
                     ofType:(NSString *)typeName 
                      error:(NSError * _Nullable *)outError
{
    _package = fileWrapper;
    [self setFiles];
    return YES;
}

- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError **)outError
{
    if (!_package) {
        [self markDirty];
        NSFileWrapper *contentsFileWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Files" : _device.files }];
        _package = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Contents" : contentsFileWrapper }];
        [self setFiles];
    }
    return _package;
}

- (void)clearOutput:(OutputType)output
{
    [((output == CTBuild) ? buildOutput : consoleOutput) setString: @""];
}

- (IBAction)renameDevice:(id)sender
{
    NSString* name = [_fileBrowser getNewDeviceName];
    if (name) {
        [_device renameDevice:name];
    }
}

- (IBAction)reloadFiles:(id)sender {
    [self reloadFiles];
}

- (void)reloadFiles
{
    [_fileBrowser reloadFilesForDevice:_device];
}

- (IBAction)upload:(id)sender {
    [_device mirrorFiles];
}

@end
