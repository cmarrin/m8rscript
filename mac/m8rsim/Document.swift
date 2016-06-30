//
//  Document.swift
//  Bar
//
//  Created by Chris Marrin on 6/28/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

import Cocoa

@objc class Document: NSDocument, NSTextStorageDelegate {

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
    var _script: M8RScript? = nil

    override init() {
        super.init()
        _script = M8RScript(document: self)
    }

    override func awakeFromNib() {
        sourceEditor.font = _font
        consoleOutput.font = _font
        buildOutput.font = _font
        sourceEditor.lnv_setUpLineNumberView()
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

    override func data(ofType typeName: String) throws -> Data {
        return (sourceEditor.string?.data(using: String.Encoding.utf8))!
    }

    override func read(from data: Data, ofType typeName: String) throws {
        _source = String(data: data as Data, encoding:String.Encoding.utf8)!
        if ((sourceEditor) != nil) {
            sourceEditor.string = _source;
        }
    }

    override func validateToolbarItem(_ item: NSToolbarItem?) -> Bool {
        if item == runButton {
            return _script!.canRun();
        }
        if item == pauseButton {
            return _script!.canPause();
        }
        if item == stopButton {
            return _script!.canStop();
        }
        return true;
    }

    func textStorageDidProcessEditing(notification: NSNotification!){
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

    @IBAction func build(_ sender: AnyObject) {
        _script?.build(sourceEditor.string)
    }
    
    @IBAction func run(_ sender: AnyObject) {
        _script?.run()
    }

    @IBAction func pause(_ sender: AnyObject) {
        _script?.pause()
    }

    @IBAction func stop(_ sender: AnyObject) {
        _script?.stop()
    }
    
    func output(message: String, isBuild: Bool)
    {
        if isBuild {
            outputView.tabViewItem(at: 1)
        } else {
            outputView.tabViewItem(at: 0)
        }
        let view = isBuild ? buildOutput : consoleOutput
        let string = "\(view?.string)\(message)"
        view?.string = string
        view?.scrollRangeToVisible(NSMakeRange((view!.string?.characters.count)!, 0));
        view?.needsDisplay = true;
    }

}


