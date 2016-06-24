//
//  AppDelegate.swift
//  m8rsim
//
//  Created by Chris Marrin on 6/23/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {



    func applicationDidFinishLaunching(aNotification: NSNotification) {
        // Insert code here to initialize your application
    }

    func applicationWillTerminate(aNotification: NSNotification) {
        // Insert code here to tear down your application
    }


    @IBAction func `import`(sender: AnyObject)
    {
        let openPanel: NSOpenPanel = NSOpenPanel()
        openPanel.allowedFileTypes = ["m8r"]
        if (openPanel.runModal() == NSModalResponseOK) {
            NSDocumentController.shared().currentDocument!.doImport(openPanel.urls[0])
        }
        
        // Get the path to the file chosen in the NSOpenPanel
//        var path = fileDialog.URL?.path
//        
//        // Make sure that a path was chosen
//        if (path != nil) {
//            var err = NSError?()
//            let text = String(contentsOfFile: path!, encoding: NSUTF8StringEncoding, error: &err)
//            
//            if !(err != nil) {
//                NSLog(text!)
//            }
//        }
    }
}

