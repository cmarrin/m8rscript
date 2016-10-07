//
//  SimulationView.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "SimulationView.h"

#import "Document.h"
#import "Engine.h"

@interface SimulationView ()
{
    __weak IBOutlet NSButton *led0;
    __weak IBOutlet NSButton *led1;
    __weak IBOutlet NSButton *led2;
}

- (void)updateLEDs:(uint16_t) state;

@end

@implementation SimulationView

- (void)updateLEDs:(uint16_t) state
{
    [led0 setState: (state & 0x01) ? NSOnState : NSOffState];
    [led0 setNeedsDisplay:YES];
    [led1 setState: (state & 0x02) ? NSOnState : NSOffState];
    [led1 setNeedsDisplay:YES];
    [led2 setState: (state & 0x04) ? NSOnState : NSOffState];
    [led2 setNeedsDisplay:YES];
}

@end
