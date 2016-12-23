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
#import <MGSFragaria/MGSFragaria.h>

#import <stdarg.h>

@interface Document ()
{
    IBOutlet NSView* editView;
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
    NSString* _selectedFilename;
    
    Device* _device;
    SimulationView* _simulationView;
    FileBrowser* _fileBrowser;
    
    NSFileWrapper* _package;
    
    MGSFragaria* _fragaria;

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
    [consoleOutput setFont:_font];
    [buildOutput setFont:_font];
    [consoleOutput setVerticallyResizable:YES];
    [buildOutput setVerticallyResizable:YES];
    
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

- (void)windowControllerDidLoadNib:(NSWindowController *) aController
{
    [super windowControllerDidLoadNib:aController];

	// create an instance
	_fragaria = [[MGSFragaria alloc] init];
	
	[_fragaria setObject:self forKey:MGSFODelegate];
	
	// define our syntax definition
	[self setSyntaxDefinition:@"Objective-C"];
	
	// embed editor in editView
	[_fragaria embedInView:editView];
	
    //
	// assign user defaults.
	// a number of properties are derived from the user defaults system rather than the doc spec.
	//
	// see MGSFragariaPreferences.h for details
	//
    if (NO) {
        [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:YES] forKey:MGSFragariaPrefsAutocompleteSuggestAutomatically];
        [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:NO] forKey:MGSFragariaPrefsLineWrapNewDocuments];
    }
	
	// define initial document configuration
	//
	// see MGSFragaria.h for details
	//
    if (YES) {
        [_fragaria setObject:[NSNumber numberWithBool:YES] forKey:MGSFOIsSyntaxColoured];
        [_fragaria setObject:[NSNumber numberWithBool:YES] forKey:MGSFOShowLineNumberGutter];
    }

    // set text
	[_fragaria setString:@"// We Don't need the future"];
	

	// access the NSTextView
	NSTextView *textView = [_fragaria objectForKey:ro_MGSFOTextView];
	
#pragma unused(textView)
	
}

-(BOOL) validateToolbarItem:(NSToolbarItem*) item
{
    if (item == buildButton) {
        return [_source length] && [[_selectedFilename pathExtension] isEqualToString:@"m8r"];
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
    [_device buildFile:_selectedFilename];
}

- (IBAction)run:(id)sender
{
    [self clearOutput:CTConsole];
    [_device runFile:_selectedFilename];
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
    [_device saveBinary:_selectedFilename];
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
    _selectedFilename = @"";
//    if (sourceEditor) {
//        [sourceEditor setString:_source];
//    }
    [_device clearContents];
}

- (void)setContents:(NSData*)contents withName:(NSString*)name;
{
    [self clearContents];
    _selectedFilename = name;
    
    NSString* source = [[NSString alloc] initWithData:contents encoding:NSUTF8StringEncoding];
    if (source) {
        _source = source;
//        if (sourceEditor) {
//            [sourceEditor setString:_source];
//        }
        return;
    }
    
    NSImage* image = [[NSImage alloc]initWithData:contents];
    if (image) {
        [self setImage:image];
        return;
    }
    
    if ([[name pathExtension] isEqualToString:@"m8rb"]) {
        [self clearOutput:CTBuild];
        [_device buildFile:name];
    }
}

- (void)setImage:(NSImage*)image
{
    NSTextAttachmentCell *attachmentCell = [[NSTextAttachmentCell alloc] initImageCell:image];
    NSTextAttachment *attachment = [[NSTextAttachment alloc] init];
    [attachment setAttachmentCell: attachmentCell ];
    NSAttributedString *attributedString = [NSAttributedString  attributedStringWithAttachment: attachment];
//    [[sourceEditor textStorage] setAttributedString:attributedString];
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
    
    if (_selectedFilename.length) {
        NSFileWrapper* files = (_package.fileWrappers && _package.fileWrappers[@"Contents"]) ?
                                    _package.fileWrappers[@"Contents"].fileWrappers[@"Files"] : 
                                    nil;
        if (files) {
            [files removeFileWrapper:files.fileWrappers[_selectedFilename]];
            [files addRegularFileWithContents:[string dataUsingEncoding:NSUTF8StringEncoding] preferredFilename:_selectedFilename];
        }
    }
    
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

#pragma mark -
#pragma mark Syntax definition handling

/*
 
 - setSyntaxDefinition:
 
 */

- (void)setSyntaxDefinition:(NSString *)name
{
	[_fragaria setObject:name forKey:MGSFOSyntaxDefinitionName];
}

/*
 
 - syntaxDefinition
 
 */
- (NSString *)syntaxDefinition
{
	return [_fragaria objectForKey:MGSFOSyntaxDefinitionName];
	
}

#pragma mark -
#pragma mark NSTextDelegate
/*
 
 - textDidChange:
 
 fragaria delegate method
 
 */
- (void)textDidChange:(NSNotification *)notification
{
	#pragma unused(notification)
	
	NSWindowController *controller = [[self windowControllers] objectAtIndex:0];
	
	[controller setDocumentEdited:YES];
}

/*
 
 - textDidBeginEditing:
 
 */
- (void)textDidBeginEditing:(NSNotification *)aNotification
{
	NSLog(@"notification : %@", [aNotification name]);
}

/*
 
 - textDidEndEditing:
 
 */
- (void)textDidEndEditing:(NSNotification *)aNotification
{
	NSLog(@"notification : %@", [aNotification name]);
}

/*
 
 - textShouldBeginEditing:
 
 */
- (BOOL)textShouldBeginEditing:(NSText *)aTextObject
{
#pragma unused(aTextObject)
	
	return YES;
}

/*
 
 - textShouldEndEditing:
 
 */
- (BOOL)textShouldEndEditing:(NSText *)aTextObject
{
	#pragma unused(aTextObject)
	
	return YES;
}

@end
