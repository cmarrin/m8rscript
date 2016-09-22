//
//  DocumentController.m
//  m8rscript
//
//  Created by Chris Marrin on 9/21/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

#import "DocumentController.h"

@interface DocumentController ()

@end

@implementation DocumentController

- (id) init
{
    if (self = [super init]) {
    }
    return self;
}

//-(void)openDocument:(id)sender
//{
//    NSLog(@"Open");
//}
//
//- (void)openDocumentWithContentsOfURL:(NSURL *)url
//                              display:(BOOL)displayDocument 
//                    completionHandler:(void (^)(NSDocument *document, BOOL documentWasAlreadyOpen, NSError *error))completionHandler
//{
//
//}
//
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)theApplication {
    return NO;
}

@end
