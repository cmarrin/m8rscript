//
//  FileBrowser.m
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#import "FileBrowser.h"

#import "Document.h"
#import "FastSocket.h"

@interface FileBrowser ()
{
    __weak IBOutlet NSTableView *fileListView;
    __weak IBOutlet NSPopUpButton *fileSourceButton;

    Document* _document;

    NSMutableArray* _fileList;
    NSFileWrapper* _files;

    NSNetServiceBrowser* _netServiceBrowser;
    NSMutableArray<NSDictionary*>* _devices;
    NSDictionary* _currentDevice;
}

@end

@implementation FileBrowser

- (instancetype)initWithDocument:(Document*) document {
    self = [super init];
    if (self) {
        _document = document;

        _fileList = [[NSMutableArray alloc] init];
        _devices = [[NSMutableArray alloc] init];
     }
    return self;
}

- (void)dealloc
{
}

- (void)awakeFromNib
{
    // Clear the fileSourceButton
    assert(fileSourceButton.numberOfItems > 0);
    [fileSourceButton selectItemAtIndex:0];
    while (fileSourceButton.numberOfItems > 1) {
        [fileSourceButton removeItemAtIndex:1];
    }
    
    _netServiceBrowser = [[NSNetServiceBrowser alloc] init];
    [_netServiceBrowser setDelegate: (id) self];
    [_netServiceBrowser searchForServicesOfType:@"_m8rscript_shell._tcp." inDomain:@"local."];
}

- (BOOL)isFileSourceLocal
{
    return [fileSourceButton.selectedItem.title isEqualToString:@"Local Files"];
}

- (void)setFiles:(NSFileWrapper*)files
{
    _files = files;
    [self reloadFiles];
}

- (NSFileWrapper*)filesFileWrapper
{
    if (!_files) {
        [_document markDirty];
        _files = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ }];
    }
    return _files;
}

- (IBAction)fileSelected:(id)sender
{
    NSIndexSet* indexes = ((NSTableView*) sender).selectedRowIndexes;
    if (indexes.count != 1) {
        // clear text editor
        [_document setSource:@""];
    } else {
        NSString* name = _fileList[fileListView.selectedRow][@"name"];
        [_document setSource:[[NSString alloc] initWithData:
            _files.fileWrappers[name].regularFileContents encoding:NSUTF8StringEncoding]];
    }
}

static void flushToPrompt(FastSocket* socket)
{
    while(1) {
        char c;
        long count = [socket receiveBytes:&c count:1];
        if (count != 1 || c == '>') {
            break;
        }
    }
}

static NSString* receiveToPrompt(FastSocket* socket)
{
    NSMutableString* s = [NSMutableString string];
    char c[2];
    c[1] = '\0';
    while(1) {
        long count = [socket receiveBytes:c count:1];
        if (count != 1 || c[0] == '>') {
            break;
        }
        [s appendString:[NSString stringWithUTF8String:c]];
    }
    return s;
}

- (NSString*)loadFileListFromHost:(NSString*)hostname port:(uint16_t) port
{
    NSString* portString = [NSNumber numberWithInt:port].stringValue;
    FastSocket* socket = [[FastSocket alloc] initWithHost:hostname andPort:portString];
    [socket connect];
    [socket setTimeout:5];
    flushToPrompt(socket);
    NSData* data = [@"ls\r\n" dataUsingEncoding:NSUTF8StringEncoding];
    long count = [socket sendBytes:data.bytes count:data.length];
    assert(count == data.length);
    
    NSString* s = receiveToPrompt(socket);
    return s;
}

- (void)reloadFiles
{
    [_fileList removeAllObjects];
    if (_currentDevice) {
        // load files from the device
        NSNetService* service = _currentDevice[@"service"];
        NSString* fileString = [self loadFileListFromHost:service.hostName port:service.port];
        if (fileString && fileString.length > 0 && [fileString characterAtIndex:0] == ' ') {
            fileString = [fileString substringFromIndex:1];
        }
        
        NSArray* lines = [fileString componentsSeparatedByString:@"\n"];
        for (NSString* line in lines) {
            NSArray* elements = [line componentsSeparatedByString:@":"];
            if (elements.count != 3 || ![elements[0] isEqualToString:@"file"]) {
                continue;
            }
            
            [_fileList addObject:@{ @"name" : elements[1], @"size" : elements[2] }];
        }
    } else {
        if (!_files) {
            return;
        }
        
        for (NSString* name in _files.fileWrappers) {
            NSFileWrapper* file = _files.fileWrappers[name];
            if (file && file.regularFile) {
                [_fileList addObject:@{ @"name" : name, @"size" : @(_files.fileWrappers[name].regularFileContents.length) }];
            }
        }
    }
    
    [_fileList sortUsingComparator:^NSComparisonResult(NSDictionary* a, NSDictionary* b) {
        return [a[@"name"] compare:b[@"name"]];
    }];
    [fileListView reloadData];
}

- (void)addFiles
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setPrompt:@"Add"];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL*  url = [[panel URLs] objectAtIndex:0];
            NSString* toName = url.lastPathComponent;
            
            NSFileWrapper* files = self.filesFileWrapper;
            if (files && files.fileWrappers[toName]) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert addButtonWithTitle:@"Cancel"];
                [alert addButtonWithTitle:@"OK"];
                [alert setMessageText:[NSString stringWithFormat:@"%@ exists, overwrite?", toName]];
                [alert setInformativeText:@"This operation cannot be undone."];
                [alert setAlertStyle:NSWarningAlertStyle];
                if ([alert runModal] == NSModalResponseCancel) {
                    return;
                }
                
                [files removeFileWrapper:files.fileWrappers[toName]];
            }
            
            [_document markDirty];
            NSFileWrapper* file  = [[NSFileWrapper alloc] initRegularFileWithContents:[NSData dataWithContentsOfURL:url]];
            file.preferredFilename = toName;
            [files addFileWrapper:file];
            [self reloadFiles];
        }
    }];
}

- (NSString*)trimTrailingDot:(NSString*)s
{
    if ([s characterAtIndex:s.length - 1] == '.') {
        return [s substringToIndex:s.length - 1];
    }
    return s;
}

- (NSNetService*)serviceFromDevice:(NSDictionary*)device
{
    return (NSNetService*) (device[@"service"]);
}

- (NSDictionary*) findService:(NSString*)hostname
{
    for (NSDictionary* service in _devices) {
        if ([hostname isEqualToString:[self trimTrailingDot:[self serviceFromDevice:service].hostName]]) {
            return service;
        }
    }
    return nil;
}

- (IBAction)changeFileSource:(id)sender
{
    _currentDevice = [self findService:[(NSPopUpButton*)sender titleOfSelectedItem]];
    [self reloadFiles];
}

- (void)removeFiles
{
    NSIndexSet* indexes = fileListView.selectedRowIndexes;
    if (!indexes.count) {
        return;
    }
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    if (indexes.count == 1) {
        [alert setMessageText:@"Delete this file?"];
    } else {
        [alert setMessageText:@"Delete these files?"];
    }
    [alert setInformativeText:@"This operation cannot be undone."];
    [alert setAlertStyle:NSWarningAlertStyle];
    if ([alert runModal] == NSModalResponseCancel) {
        return;
    }

    NSUInteger i = 0;
    NSFileWrapper* files = self.filesFileWrapper;
    if (!files) {
        return;
    }
    
    for (NSDictionary* entry in _fileList) {
        if ([indexes containsIndex:i++]) {
            NSFileWrapper* d = files.fileWrappers[[entry objectForKey:@"name"]];
            if (d) {
                [_document markDirty];
                [files removeFileWrapper:d];
            }
        }
    }
    [fileListView deselectAll:self];
    [self reloadFiles];
}

- (void)upload
{
}

// fileListView dataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [_fileList count];
}

// TableView delegagte
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    return [[_fileList objectAtIndex:rowIndex] objectForKey:aTableColumn.identifier];
}

- (void)tableView:(NSTableView *)aTableView
   setObjectValue:(id)anObject
   forTableColumn:(NSTableColumn *)aTableColumn
              row:(NSInteger)rowIndex
{
    return;
}

// NSNetServiceBrowser delegate
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser
           didFindService:(NSNetService *)netService
               moreComing:(BOOL)moreServicesComing
{
    [_devices addObject:@{
        @"service" : netService
    }];
        
    [netService setDelegate:(id) self];
    NSLog(@"*** Found service: %@\n", netService);
    [netService resolveWithTimeout:10];
}

-(void)netServiceBrowser:(NSNetServiceBrowser *)browser didNotSearch:(NSDictionary<NSString *,NSNumber *> *)errorDict {
    NSLog(@"not search ");
}

- (void)netServiceDidResolveAddress:(NSNetService *)sender
{
    NSString* hostName = [self trimTrailingDot:sender.hostName];
    [fileSourceButton addItemWithTitle:hostName];
    [fileSourceButton setNeedsDisplay];
}

- (void)netService:(NSNetService *)sender 
     didNotResolve:(NSDictionary<NSString *,NSNumber *> *)errorDict
{
    NSLog(@"********* Did not resolve: %@\n", errorDict);
}

@end
