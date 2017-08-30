//
//  m8rsim_xpc.h
//  m8rsim_xpc
//
//  Created by Chris Marrin on 7/4/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "m8rsim_xpcProtocol.h"

// This object implements the protocol which we have defined. It provides the actual behavior for the service. It is 'exported' by the service to make it available to the process hosting the service over an NSXPCConnection.
@interface m8rsim_xpc : NSObject <NSXPCListenerDelegate, m8rsim_xpcProtocol>
@end
