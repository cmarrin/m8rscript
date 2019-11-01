//
//  SimulationView.m
//  m8rsim
//
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#import "SimulationView.h"

@interface SimulationView ()
{
    __weak IBOutlet NSView *GPIOView;
}

@end

@implementation SimulationView

- (void)updateGPIOState:(uint32_t) state withMode:(uint32_t) mode
{
    for (NSBox* gpio in GPIOView.subviews) {
        NSButton* ledButton = gpio.contentView.subviews[0];
        NSButton* pushButton = gpio.contentView.subviews[1];
        NSButton* activeHighButton = gpio.contentView.subviews[2];
        
        NSInteger i = [gpio.title integerValue];
        if (i < 0 || i > 16) {
            continue;
        }
        
        BOOL isOutput = (mode & (1 << i)) != 0;
        BOOL isHigh = (state & (1 << i)) != 0;
        BOOL isActiveHigh = activeHighButton.state == NSControlStateValueOn;
        
        ledButton.state = isHigh ^ !isActiveHigh;
        ledButton.enabled = isOutput;
        pushButton.enabled = !isOutput;
    }
        
    [GPIOView setNeedsDisplay:YES];
}

@end
