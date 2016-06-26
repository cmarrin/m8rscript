//
//  Document.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Document.h"

#import "Parser.h"

#include <iostream>
#import <sstream>

@interface Document ()
{
    IBOutlet NSTextView* sourceEditor;
    __unsafe_unretained IBOutlet NSTextView *consoleOutput;
    __unsafe_unretained IBOutlet NSTextView *buildOutput;
    
    NSString* _source;
    NSFont* _font;
    
    m8r::Program* _program;
}
@end

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
        _font = [NSFont fontWithName:@"Menlo Regular" size:12];
    }
    return self;
}

- (void)awakeFromNib
{
    [sourceEditor setFont:_font];
    [consoleOutput setFont:_font];
    [buildOutput setFont:_font];
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

void print(const char* s) { std::cout << s; }

- (IBAction)build:(id)sender {
    _program = nullptr;
    
    m8r::StringStream stream([sourceEditor.string UTF8String]);
    m8r::Parser parser(&stream, print);
    addTextToOutput(buildOutput, @"Parsing finished...\n");

    if (parser.nerrors()) {
        addTextToOutput(buildOutput, [NSString stringWithFormat:@"***** %d errors\n", parser.nerrors()]);
    } else {
        addTextToOutput(buildOutput, @"0 errors. Ready to run\n");
        _program = parser.program();
    }
    
    m8r::ExecutionUnit eu(print);
    m8r::String codeString = eu.generateCodeString(_program);
    
    addTextToOutput(buildOutput, @"\n*** Start Generated Code ***\n\n");
    addTextToOutput(buildOutput, [NSString stringWithUTF8String:codeString.c_str()]);
    addTextToOutput(buildOutput, @"\n*** End of Generated Code ***\n\n");
}

- (IBAction)run:(id)sender {
    m8r::ExecutionUnit eu(print);
    eu.run(_program);
}

@end
