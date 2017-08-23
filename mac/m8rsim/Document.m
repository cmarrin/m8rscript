//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "NSTextView+JSDExtensions.h"

#import "AppDelegate.h"
#import "Device.h"
#import "SimulationView.h"
#import "FileBrowser.h"
#import <MGSFragaria/MGSFragaria.h>
#import <MGSFragaria/MGSTextMenuController.h>

#import <stdarg.h>

@interface Document ()
{
    IBOutlet NSView* editView;
    IBOutlet NSImageView* imageView;
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
    
    __weak IBOutlet NSButton *enableDebugButton;
    __weak IBOutlet NSTextField *freeMemoryLabel;
    __weak IBOutlet NSTextField *numObjectAllocationsLabel;
    __weak IBOutlet NSTextField *numStringAllocationsLabel;
    __weak IBOutlet NSTextField *numOtherAllocationsLabel;
    NSMenuItem *commentSelectionMenuItem;
    
    NSString* _source;
    NSFont* _font;
    NSString* _selectedFilename;
    NSData* _selectedContents;
    
    Device* _device;
    SimulationView* _simulationView;
    FileBrowser* _fileBrowser;
    
    NSURL* _packageURL;
    
    MGSFragaria* _fragaria;
    NSImageView* _imageView;
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
    
    enableDebugButton.state = NSOnState;
    [self setFreeMemory:45600 numObj:42 numStr:43 numOther:44];

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
    
    commentSelectionMenuItem = ((AppDelegate*)[NSApplication sharedApplication].delegate).commentSelectionMenuItem;
}

- (void)windowControllerDidLoadNib:(NSWindowController *) aController
{
    [super windowControllerDidLoadNib:aController];
    
    _imageView = [[NSImageView alloc] init];

	// create an instance
	_fragaria = [[MGSFragaria alloc] init];
	
	[_fragaria setObject:self forKey:MGSFODelegate];
	
	// define our syntax definition
	[self setSyntaxDefinition:@"javascript"];
	
	// embed editor in editView
	[_fragaria embedInView:editView];

    // FIXME: Add some support for Fragaria prefs. Here's an example of setting the showInvisibleCharacters flag
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:NO] forKey:MGSFragariaPrefsShowInvisibleCharacters];
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:YES] forKey:MGSFragariaPrefsIndentWithSpaces];
	
    //
	// assign user defaults.
	// a number of properties are derived from the user defaults system rather than the doc spec.
	//
	// see MGSFragariaPreferences.h for details
	//
//        [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:YES] forKey:MGSFragariaPrefsAutocompleteSuggestAutomatically];
//        [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:NO] forKey:MGSFragariaPrefsLineWrapNewDocuments];
	
	// define initial document configuration
	//
	// see MGSFragaria.h for details
	//
    if (YES) {
        [_fragaria setObject:[NSNumber numberWithBool:YES] forKey:MGSFOIsSyntaxColoured];
        [_fragaria setObject:[NSNumber numberWithBool:YES] forKey:MGSFOShowLineNumberGutter];
    }
}

-(BOOL) validateToolbarItem:(NSToolbarItem*) item
{
    if (item == buildButton) {
        return [_device canBuild] && [_source length] && [[_selectedFilename pathExtension] isEqualToString:@"m8r"];
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
    return NO;
}

- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    SEL action = [item action];
    if (action == @selector(saveFileAs:)) {
        return _selectedContents && [_selectedFilename length] > 0;
    }
    
    if (action == @selector(shiftRightAction:) || action == @selector(shiftLeftAction:)) {
        return _selectedContents && [_selectedFilename length] > 0;
    }

    if (action == @selector(commentOrUncommentAction:)) {
        if (_selectedContents && [_selectedFilename length] > 0) {
            //FIXME: See if we should comment or uncomment
            NSRange selectedRange = [_fragaria textView].selectedRange;
            (void) selectedRange;
            
            //commentSelectionMenuItem.title = @"Comment Selection";
            commentSelectionMenuItem.title = @"Uncomment Selection";
            return YES;
        }
        return NO;
    }

    return YES;
}

- (void) setFreeMemory:(NSUInteger)size numObj:(NSUInteger)obj numStr:(NSUInteger)str numOther:(NSUInteger)oth
{
    NSString* s = [NSString stringWithFormat:@"%.1fkb", ((float) size) / 1000];
    freeMemoryLabel.stringValue = s;
    s = [NSString stringWithFormat:@"%u", (uint32_t) obj];
    numObjectAllocationsLabel.stringValue = s;
    s = [NSString stringWithFormat:@"%u", (uint32_t) str];
    numStringAllocationsLabel.stringValue = s;
    s = [NSString stringWithFormat:@"%u", (uint32_t) oth];
    numOtherAllocationsLabel.stringValue = s;
}

+ (BOOL)autosavesInPlace {
    return YES;
}

- (IBAction)shiftRightAction:(id)sender
{
    [[MGSTextMenuController sharedInstance] shiftRightAction:sender];
}

- (IBAction)shiftLeftAction:(id)sender
{
    [[MGSTextMenuController sharedInstance] shiftLeftAction:sender];
}

- (IBAction)commentOrUncommentAction:(id)sender
{
    [[MGSTextMenuController sharedInstance] commentOrUncommentAction:sender];
}

//
// Simulator Interface
//
- (IBAction)build:(id)sender
{
    [self clearOutput:CTBuild];
    BOOL debug = enableDebugButton.state == NSOnState;
    _fragaria.syntaxErrors = [_device buildFile:_selectedFilename withDebug:debug];
}

- (IBAction)run:(id)sender
{
    [self clearOutput:CTConsole];
    BOOL debug = enableDebugButton.state == NSOnState;
    [_device runFile:_selectedFilename withDebug:debug];
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

- (IBAction)addFiles:(id)sender
{
    [_fileBrowser addFiles];
}

- (IBAction)removeFiles:(id)sender
{
    [_fileBrowser removeFiles];
}

- (IBAction)saveFileAs:(id)sender
{
    assert(_selectedContents && [_selectedFilename length] > 0);
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel setNameFieldStringValue:_selectedFilename];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            [_selectedContents writeToURL:[panel URL] atomically:YES];
        }
    }];
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
}

- (IBAction)newFile:(id)sender
{
    [_fileBrowser newFile];
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
    _selectedContents = nil;
    editView.hidden = YES;
    imageView.hidden = YES;
    [_fragaria setString:_source];
    [_device clearContents];
}

- (void)setContents:(NSData*)contents withName:(NSString*)name;
{
    [self clearContents];
    _selectedFilename = name;
    _selectedContents = contents;
    
    NSString* source = [[NSString alloc] initWithData:contents encoding:NSUTF8StringEncoding];
    if (source) {
        editView.hidden = NO;
        imageView.hidden = YES;
        _source = source;
        [_fragaria setString:_source];
        return;
    }
    
    NSImage* image = [[NSImage alloc]initWithData:contents];
    if (image) {
        [self setImage:image];
        return;
    }
    
    if ([[name pathExtension] isEqualToString:@"m8rb"]) {
        [self clearOutput:CTBuild];
        [_device buildFile:name withDebug: NO];
    }
}

- (void)setImage:(NSImage*)image
{
    editView.hidden = YES;
    imageView.hidden = NO;
    imageView.image = image;
}

- (void)selectFile:(NSInteger)index
{
    [self clearContents];
    if (index < 0) {
        return;
    }
    
    [_device selectFile:index];
}

- (BOOL)saveFile:(NSString*)name withURLBase:(NSURL*)urlBase
{
    return [_device saveFile:name withURLBase:urlBase];
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

- (void)renameFileFrom:(NSString*)oldName to:(NSString*)newName
{
    return [_device renameFileFrom:oldName to:newName];
}

- (void)setDevice:(NSString*)name
{
    [_device setDevice:name];
}

- (NSString *)windowNibName {
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"Document";
}

- (BOOL)readFromURL:(NSURL *)url 
             ofType:(NSString *)typeName 
              error:(NSError * _Nullable *)outError
{
    _packageURL = [url URLByAppendingPathComponent:@"Contents/Files"];
    [self reloadFiles];
    return YES;
}

- (BOOL)writeToURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError * _Nullable *)outError
{
//    if ([_device saveChangedFiles]) {
//        [self updateChangeCount:NSChangeCleared];
//        return YES;
//    }
//    
//    BOOL result = [super writeToURL:url ofType:typeName error:outError];
//    return result;
    return YES;
}

//- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError **)outError
//{
//    if (!_package) {
//        [self markDirty];
//        NSFileWrapper *contentsFileWrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Files" : _device.files }];
//        _package = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ @"Contents" : contentsFileWrapper }];
//        [self setFiles];
//    }
//    return _package;
//}

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
    [_fileBrowser reloadFilesWithURL:_packageURL forDevice:_device];
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
//    NSTextStorage *textStorage = [notification.object textStorage];
//    NSString *string = textStorage.string;
    
    if (_selectedFilename.length) {
//        NSFileWrapper* files = (_package.fileWrappers && _package.fileWrappers[@"Contents"]) ?
//                                    _package.fileWrappers[@"Contents"].fileWrappers[@"Files"] : 
//                                    nil;
//        if (files) {
//            [files removeFileWrapper:files.fileWrappers[_selectedFilename]];
//            [files addRegularFileWithContents:[string dataUsingEncoding:NSUTF8StringEncoding] preferredFilename:_selectedFilename];
//        }
    }
    [self reloadFiles];
    [self updateChangeCount:NSChangeDone];
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
	
	NSLog(@"textShouldBeginEditing");
	return YES;
}

/*
 
 - textShouldEndEditing:
 
 */
- (BOOL)textShouldEndEditing:(NSText *)aTextObject
{
	#pragma unused(aTextObject)
	
	NSLog(@"textShouldEndEditing");
	return YES;
}

@end
