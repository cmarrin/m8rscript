//
//  Simulator.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "Simulator.h"

#import "Document.h"
#import "Engine.h"

@interface Simulator ()
{
    __weak IBOutlet NSButton *led0;
    __weak IBOutlet NSButton *led1;
    __weak IBOutlet NSButton *led2;

    Document* _document;
    void* _engine;
}

- (void)updateLEDs:(uint16_t) state;

@end

@implementation Simulator

// C interface to Engine
void Simulator_vprintf(void* simulator, const char* s, va_list args, bool isBuild)
{
    NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
    dispatch_async(dispatch_get_main_queue(), ^{
        Document* document = ((__bridge Simulator*) simulator)->_document;
        [document outputMessage:string to:isBuild ? CTBuild : CTConsole];
    });
}

void Simulator_updateGPIOState(void* simulator, uint16_t mode, uint16_t state)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [(__bridge Simulator*) simulator updateLEDs:state];
    });
}

- (instancetype)initWithDocument:(Document*) document {
    self = [super init];
    if (self) {
        _document = document;
        _engine = Engine_createEngine((__bridge void *) self);
     }
    return self;
}

- (void)dealloc
{
    Engine_deleteEngine(_engine);
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
    return Engine_canRun(_engine);
}

- (BOOL)canStop
{
    return Engine_canStop(_engine);
}

- (void)importBinary:(const char*)filename
{
    Engine_importBinary(_engine, filename);
}

- (void)exportBinary:(const char*)filename
{
    Engine_exportBinary(_engine, filename);
}

- (void)build:(const char*) source withName:(NSString*) name
{
    [_document clearOutput:CTBuild];
    Engine_build(_engine, source, name.UTF8String);
}

- (void)run
{
    [_document clearOutput:CTConsole];
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
    dispatch_async(queue, ^() {
        Engine_run(_engine);
    });
}

- (void)pause
{
    Engine_pause(_engine);
}

- (void)stop
{
    Engine_stop(_engine);
}

- (void)simulate
{
    Engine_simulate(_engine);
}

@end
