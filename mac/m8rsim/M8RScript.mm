//
//  M8RScript.mm
//  m8rscript
//
//  Created by Chris Marrin on 6/28/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "m8rsim-Swift.h"
#import "Parser.h"
#import "CodePrinter.h"
#import "Printer.h"

#include <iostream>
#import <sstream>

class MyPrinter;

@interface M8RScript : NSObject  {
    MyPrinter* _printer;
    m8r::ExecutionUnit* _eu;
    m8r::Program* _program;
    bool _running;
}

@end

class MyPrinter : public m8r::Printer
{
public:
    MyPrinter(Document* document) : _document(document), _isBuild(true) { }
    
    virtual void print(const char* s) const override
    {
        NSString* string = [NSString stringWithUTF8String:s];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_document outputWithMessage:string isBuild: _isBuild];
        });
    }
    
    void setToBuild(bool b) { _isBuild = b; }
    
private:
    Document* _document;
    bool _isBuild;
};

@implementation M8RScript

- (instancetype)initWithDocument:(Document*) document {
    _printer = new MyPrinter(document);
    return self;
}

- (void)dealloc {
    delete _printer;
}

- (BOOL)canRun { return _program && !_running; }
- (BOOL)canStop { return _program && _running; }
- (BOOL)canPause { return NO; }

- (void)build:(NSString*)string {
    _program = nullptr;
    _running = false;
    _printer->setToBuild(true);
     
    _printer->print([[NSString stringWithFormat:@"Building %@\n", [self displayName]] UTF8String]);

    m8r::StringStream stream([string UTF8String]);
    m8r::Parser parser(&stream, _printer);
    //[self outputMessage:@"Parsing finished...\n" toBuild:YES];

    if (parser.nerrors()) {
        //[self outputMessage:[NSString stringWithFormat:@"***** %d error%s\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s"] toBuild:YES];
    } else {
        //[self outputMessage:@"0 errors. Ready to run\n" toBuild:YES];
        _program = parser.program();

        m8r::CodePrinter codePrinter(_printer);
        m8r::String codeString = codePrinter.generateCodeString(_program);
        
        //[self outputMessage:@"\n*** Start Generated Code ***\n\n" toBuild:YES];
        //[self outputMessage:[NSString stringWithUTF8String:codeString.c_str()] toBuild:YES];
        //[self outputMessage:@"\n*** End of Generated Code ***\n\n" toBuild:YES];
    }
}

- (void)run {
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    
    //[self outputMessage:@"*** Program started...\n\n" toBuild:NO];
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        _printer->setToBuild(false);
        NSTimeInterval timeInSeconds = [[NSDate date] timeIntervalSince1970];
        _eu->run(_program);
        timeInSeconds = [[NSDate date] timeIntervalSince1970] - timeInSeconds;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            //[self outputMessage:[NSString stringWithFormat:@"\n\n*** Finished (run time:%fms)\n", timeInSeconds * 1000] toBuild:NO];
            _running = false;
        });
    });
}

- (void)pause {
}

- (void)stop {
    if (!_running) {
        assert(0);
        return;
    }
    _eu->requestTermination();
    _running = false;
    //[self outputMessage:@"*** Stopped\n" toBuild:NO];
    return;
}

@end
