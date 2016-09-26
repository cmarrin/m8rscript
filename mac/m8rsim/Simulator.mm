//
//  Simulator.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Simulator.h"

#import "Application.h"
#import "Document.h"
#import "Parser.h"
#import "CodePrinter.h"
#import "ExecutionUnit.h"
#import "SystemInterface.h"

class MySystemInterface;

@interface Simulator ()
{
    __weak IBOutlet NSButton *led0;
    __weak IBOutlet NSButton *led1;
    __weak IBOutlet NSButton *led2;

    Document* _document;
    MySystemInterface* _system;
    m8r::ExecutionUnit* _eu;
    m8r::Program* _program;
    m8r::Application* _application;
    bool _running;
}

- (void)outputMessage:(NSString*) message to:(OutputType) output;
- (void)updateLEDs:(uint16_t) state;

@end

class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface(Simulator* sim) : _simulator(sim), _isBuild(true) { }
    
    virtual void printf(const char* s, ...) const override
    {
        va_list args;
        va_start(args, s);
        NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
        dispatch_async(dispatch_get_main_queue(), ^{
            [_simulator outputMessage:string to: _isBuild ? CTBuild : CTConsole];
        });
    }

    virtual int read() const override
    {
        return -1;
    }

    virtual void updateGPIOState(uint16_t mode, uint16_t state) override
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [_simulator updateLEDs:state];
        });
    }
    
    void setToBuild(bool b) { _isBuild = b; }
    
private:
    Simulator* _simulator;
    bool _isBuild;
};

@implementation Simulator

- (instancetype)initWithDocument:(Document*) document {
    self = [super init];
    if (self) {
        _document = document;
        _system = new MySystemInterface(self);
        _eu = new m8r::ExecutionUnit(_system);
     }
    return self;
}

- (void)dealloc
{
    delete _eu;
    delete _system;
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

- (BOOL)canRun
{
    return _program && !_running;
}

- (BOOL)canStop
{
    return _program && _running;
}

- (void)outputMessage:(NSString*) message to:(OutputType) output
{
    [_document outputMessage:message to:output];
}

- (void)importBinary:(const char*)filename
{
    m8r::FileStream stream(filename, "r");
    _program = new m8r::Program(_system);
    m8r::Error error;
    if (!_program->deserializeObject(&stream, error)) {
        error.showError(_system);
    }
}

- (void)exportBinary:(const char*)filename
{
    m8r::FileStream stream(filename, "w");
    if (_program) {
        m8r::Error error;
        if (!_program->serializeObject(&stream, error)) {
            error.showError(_system);
        }
    }
}

- (void)build:(const char*) source withName:(NSString*) name
{
    _program = nullptr;
    _running = false;
    
    [_document clearOutput:CTBuild];
    _system->setToBuild(true);
    [self outputMessage:[NSString stringWithFormat:@"Building %@\n", name] to:CTBuild];

    m8r::StringStream stream(source);
    m8r::Parser parser(_system);
    parser.parse(&stream);
    [self outputMessage:@"Parsing finished...\n" to:CTBuild];

    if (parser.nerrors()) {
        [self outputMessage:[NSString stringWithFormat:@"***** %d error%s\n", 
                            parser.nerrors(), (parser.nerrors() == 1) ? "" : "s"] to:CTBuild];
    } else {
        [self outputMessage:@"0 errors. Ready to run\n" to:CTBuild];
        _program = parser.program();

        m8r::CodePrinter codePrinter(_system);
        m8r::String codeString = codePrinter.generateCodeString(_program);
        
        [self outputMessage:@"\n*** Start Generated Code ***\n\n" to:CTBuild];
        [self outputMessage:[NSString stringWithUTF8String:codeString.c_str()] to:CTBuild];
        [self outputMessage:@"\n*** End of Generated Code ***\n\n" to:CTBuild];
    }
}

- (void)run
{
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    
    [_document clearOutput:CTConsole];
    [self outputMessage:@"*** Program started...\n\n" to:CTConsole];
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
                [NSThread sleepForTimeInterval:(NSTimeInterval) delay / 1000];
            }
        }
        
        timeInSeconds = [[NSDate date] timeIntervalSince1970] - timeInSeconds;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [self outputMessage:[NSString stringWithFormat:@"\n\n*** Finished (run time:%fms)\n", timeInSeconds * 1000] to:CTConsole];
            _running = false;
        });
    });
}

- (void)pause
{
}

- (void)stop
{
    if (!_running) {
        assert(0);
        return;
    }
    _eu->requestTermination();
    _running = false;
    [self outputMessage:@"*** Stopped\n" to:CTConsole];
}

- (void)simulate
{
    _application = new m8r::Application(_system);
    _program = _application->program();
    [self run];
}

@end
