//
//  Document.swift
//  Bar
//
//  Created by Chris Marrin on 6/28/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

import Cocoa

class Document: NSDocument, NSTextStorageDelegate {

    @IBOutlet var sourceEditor: NSTextView!
    @IBOutlet var consoleOutput: NSTextView!
    @IBOutlet var buildOutput: NSTextView!
    @IBOutlet weak var outputView: NSTabView!
    @IBOutlet weak var runButton: NSToolbarItem!
    @IBOutlet weak var pauseButton: NSToolbarItem!
    @IBOutlet weak var stopButton: NSToolbarItem!
    @IBOutlet weak var buildButton: NSToolbarItem!
    
    var _source = ""
    let _font = NSFont(name:"Menlo Regular", size:12)

    override init() {
        super.init()
    }

    override func awakeFromNib() {
        sourceEditor.font = _font
        consoleOutput.font = _font;
        buildOutput.font = _font;
        sourceEditor.lnv_setUpLineNumberView();
        sourceEditor.textStorage?.delegate = self
        if (!_source.isEmpty) {
            sourceEditor.string = _source
        }
}

    override class func autosavesInPlace() -> Bool {
        return true
    }

    override var windowNibName: String? {
        // Returns the nib file name of the document
        // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this property and override -makeWindowControllers instead.
        return "Document"
    }

    override func data(ofType typeName: String) throws -> NSData {
        return (sourceEditor.string?.data(usingEncoding: NSUTF8StringEncoding))!
    }

    override func read(from data: NSData, ofType typeName: String) throws {
        _source = String(data: data, encoding:NSUTF8StringEncoding)!
        if ((sourceEditor) != nil) {
            sourceEditor.string = _source;
        }
    }

    override func textStorageDidProcessEditing(notification: NSNotification) {
        let textStorage = notification.object
        textStorage!.removeAttribute(NSForegroundColorAttributeName, range:NSMakeRange(0, textStorage!.length));
//        for (NSUInteger i = 0; i < n; i++) {
//            unichar c = [string characterAtIndex:i];
//            if (c == '\\') {
//                [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor lightGrayColor] range:NSMakeRange(i, 1)];
//                i++;
//            } else if (c == '$') {
//                NSUInteger l = ((i < n - 1) && isdigit([string characterAtIndex:i+1])) ? 2 : 1;
//                [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor redColor] range:NSMakeRange(i, l)];
//                i++;
//            }
//        }
}

    @IBAction func run(sender: AnyObject) {
    }

    @IBAction func build(sender: AnyObject) {
    }
    
    @IBAction func pause(sender: AnyObject) {
    }

    @IBAction func stop(sender: AnyObject) {
    }
}

