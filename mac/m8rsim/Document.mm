//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "NSTextView+JSDExtensions.h"

#import "Parser.h"
#import "CodePrinter.h"
#import "ExecutionUnit.h"
#import "SystemInterface.h"

#include <iostream>
#include <stdarg.h>
#import <sstream>

class MySystemInterface;

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    __weak IBOutlet NSTabView *outputView;
    __weak IBOutlet NSToolbarItem *runButton;
    __weak IBOutlet NSToolbarItem *buildButton;
    __weak IBOutlet NSToolbarItem *pauseButton;
    __weak IBOutlet NSToolbarItem *stopButton;
    __weak IBOutlet NSButton *led0;
    __weak IBOutlet NSButton *led1;
    __weak IBOutlet NSButton *led2;
    
    NSString* _source;
    NSFont* _font;
    MySystemInterface* _system;
    m8r::ExecutionUnit* _eu;
    m8r::Program* _program;
    bool _running;
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
    if (item == runButton) {
        return _program && !_running;
    }
    if (item == pauseButton) {
        return NO;
    }
    if (item == stopButton) {
        return _program && _running;
    }
    return YES;
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

- (instancetype)init {
    self = [super init];
    if (self) {
        _system = new MySystemInterface(self);
        _eu = new m8r::ExecutionUnit(_system);
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
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
        runButton.enabled = YES;

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
        while (_eu->continueExecution()) ;
        
        timeInSeconds = [[NSDate date] timeIntervalSince1970] - timeInSeconds;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [self outputMessage:[NSString stringWithFormat:@"\n\n*** Finished (run time:%fms)\n", timeInSeconds * 1000] toBuild:NO];
            _running = false;
            runButton.enabled = YES;
            stopButton.enabled = NO;
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


@end
