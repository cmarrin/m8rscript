//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "NSTextView+JSDExtensions.h"

#import "Application.h"
#import "Parser.h"
#import "CodePrinter.h"
#import "ExecutionUnit.h"
#import "SystemInterface.h"
#import "MacFS.h"

#import <iostream>
#import <stdarg.h>
#import <sstream>
#import <thread>
#import <chrono>
#import <cstdio>

class MySystemInterface;

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    __weak IBOutlet NSTabView *outputView;
    __weak IBOutlet NSTabView *simView;
    __weak IBOutlet NSTableView *fileListView;
    
    __weak IBOutlet NSToolbarItem *runButton;
    __weak IBOutlet NSToolbarItem *buildButton;
    __weak IBOutlet NSToolbarItem *pauseButton;
    __weak IBOutlet NSToolbarItem *stopButton;
    __weak IBOutlet NSToolbarItem *uploadButton;
    __weak IBOutlet NSToolbarItem *simulateButton;
    __weak IBOutlet NSToolbarItem *addFileButton;
    __weak IBOutlet NSToolbarItem *removeFileButton;
    __weak IBOutlet NSToolbarItem *reloadFilesButton;
    __weak IBOutlet NSButton *led0;
    __weak IBOutlet NSButton *led1;
    __weak IBOutlet NSButton *led2;
    
    NSString* _source;
    NSFont* _font;
    MySystemInterface* _system;
    m8r::ExecutionUnit* _eu;
    m8r::Program* _program;
    m8r::Application* _application;
    bool _running;
    NSMutableArray* _fileList;
    NSNetServiceBrowser* _netServiceBrowser;
}

- (void)outputMessage:(NSString*) message toBuild:(BOOL) isBuild;
- (void)updateLEDs:(uint16_t) state;

@end

class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface(Document* document) : _document(document), _isBuild(true) { }
    
    virtual void printf(const char* s, ...) const override
    {
        va_list args;
        va_start(args, s);
        NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_document outputMessage:string toBuild: _isBuild];
        });
    }

    virtual int read() const override
    {
        return -1;
    }

    virtual void updateGPIOState(uint16_t mode, uint16_t state) override
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [_document updateLEDs:state];
        });
    }
    
    void setToBuild(bool b) { _isBuild = b; }
    
private:
    Document* _document;
    bool _isBuild;
};

@implementation Document

-(BOOL) validateToolbarItem:(NSToolbarItem*) item
{
    if (item == buildButton || item == simulateButton) {
        return YES;
    }
    if (item == runButton) {
        return _program && !_running;
    }
    if (item == stopButton) {
        return _program && _running;
    }
    if (item == addFileButton || item == removeFileButton || item == reloadFilesButton) {
        return [[simView selectedTabViewItem].identifier isEqualToString:@"files"];
    }
    return NO;
}

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

- (instancetype)init {
    self = [super init];
    if (self) {
        _system = new MySystemInterface(self);
        _eu = new m8r::ExecutionUnit(_system);
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
        
        setFileSystemPath();
        _fileList = [[NSMutableArray alloc] init];
        [self reloadFiles];

    
        _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
        [_netServiceBrowser setDelegate: (id) self];
        [_netServiceBrowser searchForServicesOfType:@"_homekit._tcp." inDomain:@"local."];	
     }
    return self;
}

- (void)dealloc
{
    delete _eu;
    delete _system;
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
}

+ (BOOL)autosavesInPlace {
    return YES;
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

- (void)outputMessage:(NSString*) message toBuild:(BOOL) isBuild
{
    if (isBuild) {
        [outputView selectTabViewItemAtIndex:1];
    } else {
        [outputView selectTabViewItemAtIndex:0];
    }
    NSTextView* view = isBuild ? buildOutput : consoleOutput;
    NSString* string = [NSString stringWithFormat: @"%@%@", view.string, message];
    [view setString:string];
    [view scrollRangeToVisible:NSMakeRange([[view string] length], 0)];
    [view setNeedsDisplay:YES];
}

- (void)updateLEDs:(uint16_t) state
{
    [led0 setState: (state & 0x01) ? NSOnState : NSOffState];
    [led0 setNeedsDisplay:YES];
    [led1 setState: (state & 0x02) ? NSOnState : NSOffState];
    [led1 setNeedsDisplay:YES];
    [led2 setState: (state & 0x04) ? NSOnState : NSOffState];
    [led2 setNeedsDisplay:YES];
}

- (IBAction)build:(id)sender {
    _program = nullptr;
    _running = false;
    
    [buildOutput setString: @""];
    
    _system->setToBuild(true);
    [self outputMessage:[NSString stringWithFormat:@"Building %@\n", [self displayName]] toBuild:YES];

    m8r::StringStream stream([sourceEditor.string UTF8String]);
    m8r::Parser parser(_system);
    parser.parse(&stream);
    [self outputMessage:@"Parsing finished...\n" toBuild:YES];

    if (parser.nerrors()) {
        [self outputMessage:[NSString stringWithFormat:@"***** %d error%s\n", 
                            parser.nerrors(), (parser.nerrors() == 1) ? "" : "s"] toBuild:YES];
    } else {
        [self outputMessage:@"0 errors. Ready to run\n" toBuild:YES];
        _program = parser.program();

        m8r::CodePrinter codePrinter(_system);
        m8r::String codeString = codePrinter.generateCodeString(_program);
        
        [self outputMessage:@"\n*** Start Generated Code ***\n\n" toBuild:YES];
        [self outputMessage:[NSString stringWithUTF8String:codeString.c_str()] toBuild:YES];
        [self outputMessage:@"\n*** End of Generated Code ***\n\n" toBuild:YES];
    }
}

- (IBAction)run:(id)sender {
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    
    [consoleOutput setString: @""];
    [self outputMessage:@"*** Program started...\n\n" toBuild:NO];
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        _system->setToBuild(false);
        NSTimeInterval timeInSeconds = [[NSDate date] timeIntervalSince1970];
        
        _eu->startExecution(_program);
        while (1) {
            int32_t delay = _eu->continueExecution();
            if (delay < 0) {
                break;
            }
            if (delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
        
        timeInSeconds = [[NSDate date] timeIntervalSince1970] - timeInSeconds;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [self outputMessage:[NSString stringWithFormat:@"\n\n*** Finished (run time:%fms)\n", timeInSeconds * 1000] toBuild:NO];
            _running = false;
        });
    });
}

- (IBAction)pause:(id)sender {
}

- (IBAction)stop:(id)sender {
    if (!_running) {
        assert(0);
        return;
    }
    _eu->requestTermination();
    _running = false;
    [self outputMessage:@"*** Stopped\n" toBuild:NO];
    return;
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

- (IBAction)simulate:(id)sender {
    _application = new m8r::Application(_system);
    _program = _application->program();
    [self run:self];
}

- (IBAction)importBinary:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setAllowedFileTypes:@[@"m8rp"]];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [[panel URLs] objectAtIndex:0];
            m8r::FileStream stream([url fileSystemRepresentation], "r");
            _program = new m8r::Program(_system);
            m8r::Error error;
            if (!_program->deserializeObject(&stream, error)) {
                error.showError(_system);
            }
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
            m8r::FileStream stream([url fileSystemRepresentation], "w");
            if (_program) {
                m8r::Error error;
                if (!_program->serializeObject(&stream, error)) {
                    error.showError(_system);
                }
            }
        }
    }];
}

// simView delegate
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
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

// NSNetServiceBrowser delegate
- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *)netServiceBrowser
{
    NSLog(@"*** netServiceBrowserWillSearch\n");
}

- (void)ne:(NSNetServiceBrowser *)netServiceBrowser
           didFindService:(NSNetService *)netService
               moreComing:(BOOL)moreServicesComing
{
    NSLog(@"*** Found service: %@\n", netService);
}

-(void)netServiceBrowser:(NSNetServiceBrowser *)browser didNotSearch:(NSDictionary<NSString *,NSNumber *> *)errorDict {
    NSLog(@"not search ");
}

@end
