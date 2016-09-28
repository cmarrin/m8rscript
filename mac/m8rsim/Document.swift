//
//  Document.swift
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

import Cocoa

@objc class Document: NSDocument, NSTextStorageDelegate
{    
    @IBOutlet weak var sourceEditor: NSTextView?
    @IBOutlet weak var consoleOutput: NSTextView?
    @IBOutlet weak var buildOutput: NSTextView?
    @IBOutlet weak var outputView: NSTabView?
    @IBOutlet weak var simContainer: NSView?
    @IBOutlet weak var filesContainer: NSView?
    @IBOutlet weak var runButton: NSToolbarItem?
    @IBOutlet weak var buildButton: NSToolbarItem?
    @IBOutlet weak var pauseButton: NSToolbarItem?
    @IBOutlet weak var stopButton: NSToolbarItem?
    @IBOutlet weak var uploadButton: NSToolbarItem?
    @IBOutlet weak var simulateButton: NSToolbarItem?
    @IBOutlet weak var addFileButton: NSToolbarItem?
    @IBOutlet weak var removeFileButton: NSToolbarItem?
    @IBOutlet weak var reloadFilesButton: NSToolbarItem?

    var _source: String! = ""
    var _font: NSFont?
        
    var _simulator: Simulator?
    var _fileBrowser: FileBrowser?
    
    var _package: FileWrapper?

    override init() {
        super.init()
        _font = NSFont(name:"Menlo Regular", size:12)
        _source = ""
    }

    override func awakeFromNib()
    {
        sourceEditor?.font = _font
        consoleOutput?.font = _font
        buildOutput?.font = _font
        sourceEditor?.showsLineNumbers = true
        sourceEditor?.isAutomaticQuoteSubstitutionEnabled = false
        sourceEditor?.textStorage?.delegate = self
        sourceEditor?.string = _source
        _simulator = Simulator(document: self)
        simContainer?.addSubview((_simulator?.view)!)
        var superFrame: NSRect = simContainer!.frame
        _simulator?.view.setFrameSize(superFrame.size)

        _fileBrowser = FileBrowser(document: self)
        filesContainer?.addSubview((_fileBrowser?.view)!)
        superFrame = filesContainer!.frame
        _fileBrowser?.view.setFrameSize(superFrame.size)
    }

    override func validateToolbarItem(_ item: NSToolbarItem) -> Bool
    {
        if (_simulator == nil) {
            return false;
        }
        if (item == buildButton || item == simulateButton) {
            return true;
        }
        if (item == runButton) {
            return (_simulator?.canRun())!;
        }
        if (item == stopButton) {
            return (_simulator?.canStop())!;
        }
        if (item == addFileButton || item == removeFileButton || item == reloadFilesButton) {
            return ((_fileBrowser?.isFileSourceLocal) != nil);
        }
        return false;
    }

    override class func autosavesInPlace() -> Bool
    {
        return true
    }

    //
    // Simulator Interface
    //
    @IBAction func build(_ sender: AnyObject)
    {
        _simulator?.build(sourceEditor?.string, withName: displayName)
    }

    @IBAction func run(_ sender: AnyObject)
    {
        _simulator?.run();
    }

    @IBAction func pause(_ sender: AnyObject)
    {
        _simulator?.pause();
    }

    @IBAction func stop(_ sender: AnyObject)
    {
        _simulator?.stop();
    }

    @IBAction func simulate(_ sender: AnyObject)
    {
        _simulator?.simulate();
    }

    @IBAction func importBinary(_ sender: AnyObject)
    {
        let panel: NSOpenPanel = NSOpenPanel()
        panel.allowedFileTypes = ["m8rp"]
        panel.begin { (result) -> Void in
            if (result == NSFileHandlingPanelOKButton) {
                self._simulator?.importBinary(panel.urls[0].path)
            }
        }
    }

    @IBAction func exportBinary(_ sender: AnyObject)
    {
        let newName: String = (fileURL?.lastPathComponent)!
        
        let panel: NSSavePanel = NSSavePanel()
        panel.nameFieldStringValue = newName;
        panel.begin { (result) -> Void in
            if (result == NSFileHandlingPanelOKButton) {
                self._simulator?.exportBinary(panel.url?.path)
            }
        }
    }

    func outputMessage(_ message: String, isBuild: Bool)
    {
        outputView?.selectTabViewItem(at:(isBuild) ? 1 : 0);
        let view: NSTextView = ((isBuild) ? buildOutput : consoleOutput)!;
        view.string = view.string! + message
        view.scrollRangeToVisible(NSRange(location:view.string!.characters.count, length:0))
        view.needsDisplay = true
    }

    func markDirty()
    {
        updateChangeCount(NSDocumentChangeType.changeReadOtherContents)
    }

    func setSource(_ source:String)
    {
        _source = source;
        if let v = sourceEditor {
            v.string = _source
        }
    }

    //
    // Text Content Interface
    //
    func textStorage(_ textStorage: NSTextStorage,
        didProcessEditing editedMask: NSTextStorageEditActions, 
                    range editedRange: NSRange, 
           changeInLength delta: Int)
    {
        let string = textStorage.string
        let n = string.characters.count
        textStorage.removeAttribute(NSForegroundColorAttributeName, range: NSRange(location: 0, length: n))
//        for c in string.characters {
//            if c == "\\" {
//                textStorage.addAttribute(NSForegroundColorAttributeName, value:Color.lightGrayColor, range:NSRange(i, 1));
//                i++;
//            } else if (c == '$') {
//                NSUInteger l = ((i < n - 1) && isdigit([string characterAtIndex:i+1])) ? 2 : 1;
//                [textStorage addAttribute:NSForegroundColorAttributeName value:[NSColor redColor] range:NSMakeRange(i, l)];
//                i++;
//            }
//        }
    }

    override var windowNibName: String?
    {
        // Returns the nib file name of the document
        // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this property and override -makeWindowControllers instead.
        return "Document"
    }

    override func data(ofType typeName: String) throws -> Data
    {
        return sourceEditor!.string!.data(using: String.Encoding.utf8)!
    }

    override func read(from data: Data, ofType typeName: String) throws
    {
        throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
    }

    override func read(from fileWrapper: FileWrapper, ofType typeName: String) throws
    {
        assert(_package == nil, "Package should not exist when readFromFileWrapper is called");
        _package = fileWrapper;
        if (_package != nil) {
            let contents = (_package!.fileWrappers != nil) ? _package!.fileWrappers?["Contents"] : nil
            if (contents != nil && contents?.fileWrappers != nil) {
                let files = contents?.fileWrappers?["Files"]
                _fileBrowser?.setFiles(files)
            }
        }
    }

    override func fileWrapper(ofType typeName: String) throws -> FileWrapper
    {
        if (_package == nil) {
            markDirty()
            let contentsFileWrapper = FileWrapper(directoryWithFileWrappers: [ "Files" : (_fileBrowser?.files)! ])
            _package = FileWrapper(directoryWithFileWrappers:[ "Contents" : contentsFileWrapper ])
        }
        return _package!;
    }

    func clearOutput(isBuild: Bool)
    {
        (isBuild ? buildOutput : consoleOutput)?.string = ""
    }

    @IBAction func addFile(_ sender: AnyObject)
    {
        _fileBrowser?.addFiles()
    }

    @IBAction func removeFile(_ sender: AnyObject)
    {
        _fileBrowser?.removeFiles()
    }

    @IBAction func reloadFiles(_ sender: AnyObject)
    {
        _fileBrowser?.reloadFiles()
    }

    @IBAction func upload(_ sender: AnyObject)
    {
        _fileBrowser?.upload()
    }
}
