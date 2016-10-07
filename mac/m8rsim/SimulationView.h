//
//  SimulationView.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class Document;

@interface SimulationView : NSViewController

- (void)updateGPIOState:(uint16_t) state withMode:(uint16_t) mode;

@end

