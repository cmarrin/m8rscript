//
//  main.m
//  m8rsim_xpc
//
//  Created by Chris Marrin on 7/4/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "m8rsim_xpc.h"

int main(int argc, const char *argv[])
{
    // Create the delegate for the service.
    m8rsim_xpc *delegate = [m8rsim_xpc new];
    
    // Set up the one NSXPCListener for this service. It will handle all incoming connections.
    NSXPCListener *listener = [NSXPCListener serviceListener];
    listener.delegate = delegate;
    
    // Resuming the serviceListener starts this service. This method does not return.
    [listener resume];
    return 0;
}
