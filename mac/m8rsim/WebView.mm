/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "webview_impl.h"
#include "WebView.h"

using namespace Sim;

static inline wv::WebView* webView(void* wv)
{
    return reinterpret_cast<wv::WebView*>(wv);
}

Sim::WebView::WebView(int width, int height, const std::string& title)
{
    wv::WebView* w = new wv::WebView(width, height, true, true, title.c_str());
    _webView = w;

    if (w->init() == -1) {
        delete w;
        _webView = nullptr;
        return;
    }
}

Sim::WebView::~WebView() 
{
    if (_webView) {
        delete webView(_webView);
    }
}

bool Sim::WebView::run()
{
    return webView(_webView)->run();
}

int wv::WebView::init()
{
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

    // Set window to be resizable
    if (resizable) {
        style |= NSWindowStyleMaskResizable;
    }

    // Initialize Cocoa window
    window = [[NSWindow alloc]
        // Initial window size
        initWithContentRect:NSMakeRect(0, 0, width, height)
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];

    // Minimum window size
    [window setContentMinSize:NSMakeSize(width, height)];

    // Position window in center of screen
    [window center];

    // Initialize WKWebView
    WKWebViewConfiguration *config = [WKWebViewConfiguration new];
    WKPreferences *prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    WKUserContentController *controller = [config userContentController];

    // Add inject script
    WKUserScript *userScript = [WKUserScript alloc];
    (void) [userScript initWithSource:[NSString stringWithUTF8String:inject.c_str()]
                       injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                       forMainFrameOnly:NO];
    [controller addUserScript:userScript];

    webview = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];

    // Add delegate methods manually in order to capture "this"
    class_replaceMethod(
        [WindowDelegate class], @selector(windowWillClose:),
        imp_implementationWithBlock([=](id self, SEL cmd, id notification) { this->exit(); }),
        "v@:@");

    class_replaceMethod(
        [WindowDelegate class],
        @selector(userContentController:didReceiveScriptMessage:),
        imp_implementationWithBlock(
            [=](id self, SEL cmd, WKScriptMessage *scriptMessage) {
                if (this->js_callback) {
                    id body = [scriptMessage body];
                    if (![body isKindOfClass:[NSString class]]) {
                        return;
                    }

                    std::string msg = [body UTF8String];
                    this->js_callback(*this, msg);
                }
            }),
            "v@:@");

    WindowDelegate *delegate = [WindowDelegate alloc];
    [controller addScriptMessageHandler:delegate name:@"webview"];
    
    // Set delegate to window
    [window setDelegate:delegate];

    // Initialize application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Sets the app as the active app
    [NSApp activateIgnoringOtherApps:YES];

    // Add webview to window
    [window setContentView:webview];

    // Display window
    [window makeKeyAndOrderFront:nil];

    // Done initialization, set properties
    init_done = true;

    setTitle(_title);
    if (fullscreen) {
        setFullscreen(true);
    }
    setBgColor(bgR, bgG, bgB, bgA);
    
    if (_url.empty()) {
        // Load index.html from the bundle
        NSURL* url = [[NSBundle mainBundle] URLForResource:@"index" withExtension: @"html"];
        [webview loadFileURL:url allowingReadAccessToURL: [url URLByDeletingLastPathComponent]];
    }
    return 0;
}

