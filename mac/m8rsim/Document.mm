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
#import "Printer.h"

#include <iostream>
#import <sstream>

class MyPrinter;

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    
    NSString* _source;
    NSFont* _font;
    MyPrinter* _printer;
    
    m8r::Program* _program;
}

- (void)outputBuildMessage:(const char*) message;

@end

class MyPrinter : public m8r::Printer
{
public:
    MyPrinter(Document* document) : _document(document) { }
    
    virtual void print(const char* s) const override
    {
        [_document outputBuildMessage:s];
    }
    
private:
    Document* _document;
};

@implementation Document

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
        _printer = new MyPrinter(self);
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
    }
    return self;
}

- (void)awakeFromNib
{
    [sourceEditor setFont:_font];
    [consoleOutput setFont:_font];
    [buildOutput setFont:_font];
    sourceEditor.ShowsLineNumbers = YES;
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

- (void)doImport:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [[panel URLs] objectAtIndex:0];
            sourceEditor.string = [NSString stringWithContentsOfURL:url encoding:kUnicodeUTF8Format error:nil];
        }
    }];
}

static void addTextToOutput(NSTextView* view, NSString* text)
{
    NSString* string = [NSString stringWithFormat: @"%@%@", view.string, text];
    [view setString:string];
    [view scrollRangeToVisible:NSMakeRange([[view string] length], 0)];
}

- (void)outputBuildMessage:(const char*) message
{
    addTextToOutput(buildOutput, [NSString stringWithUTF8String:message]);
}

- (IBAction)build:(id)sender {
    _program = nullptr;
    
    [buildOutput setString: @""];
    
    addTextToOutput(buildOutput, [NSString stringWithFormat:@"Building %@\n", [self displayName]]);

    m8r::StringStream stream([sourceEditor.string UTF8String]);
    m8r::Parser parser(&stream, _printer);
    addTextToOutput(buildOutput, @"Parsing finished...\n");

    if (parser.nerrors()) {
        addTextToOutput(buildOutput, [NSString stringWithFormat:@"***** %d error%s\n", 
                                        parser.nerrors(), (parser.nerrors() == 1) ? "" : "s"]);
    } else {
        addTextToOutput(buildOutput, @"0 errors. Ready to run\n");
        _program = parser.program();

        m8r::ExecutionUnit eu(_printer);
        m8r::String codeString = eu.generateCodeString(_program);
        
        addTextToOutput(buildOutput, @"\n*** Start Generated Code ***\n\n");
        addTextToOutput(buildOutput, [NSString stringWithUTF8String:codeString.c_str()]);
        addTextToOutput(buildOutput, @"\n*** End of Generated Code ***\n\n");
    }
}

- (IBAction)run:(id)sender {
    m8r::ExecutionUnit eu(_printer);
    eu.run(_program);
}

@end
