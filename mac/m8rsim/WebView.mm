/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "WebView.h"

#import <Cocoa/Cocoa.h>
#import <Webkit/Webkit.h>
#include <objc/objc-runtime.h>

@interface WindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
@end

@implementation WindowDelegate
- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)scriptMessage {
}
@end

using namespace Sim;

class WebViewImpl : public Sim::WebView
{
public:
    WebViewImpl(int width, int height, bool resizable, bool debug, const std::string& title);
    virtual ~WebViewImpl();
    virtual bool run() override;
    
    virtual void setTitle(const std::string& t) override;
    virtual void setFullscreen(bool fs) override;
    virtual void navigate(const std::string& u) override;
    virtual void preEval(const std::string& js) override;
    virtual void eval(const std::string& js) override;
    virtual void css(const std::string& css) override;
    virtual void exit() override;

private:
    NSWindow* _window;
    WKWebView* _webview;
};

WebViewImpl::WebViewImpl(int width, int height, bool resizable, bool debug, const std::string& title)
{
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

    // Set window to be resizable
    if (resizable) {
        style |= NSWindowStyleMaskResizable;
    }

    // Initialize Cocoa window
    _window = [[NSWindow alloc]
        // Initial window size
        initWithContentRect:NSMakeRect(0, 0, width, height)
        styleMask:style
        backing:NSBackingStoreBuffered
        defer:NO];

    // Minimum window size
    [_window setContentMinSize:NSMakeSize(width, height)];

    // Position window in center of screen
    [_window center];

    // Initialize WKWebView
    WKWebViewConfiguration *config = [WKWebViewConfiguration new];
    WKPreferences *prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    WKUserContentController *controller = [config userContentController];

    // Add inject script
    WKUserScript *userScript = [WKUserScript alloc];
    (void) [userScript initWithSource:[NSString stringWithUTF8String:_inject.c_str()]
                       injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                       forMainFrameOnly:NO];
    [controller addUserScript:userScript];

    _webview = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];

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
                if (this->_jscb) {
                    id body = [scriptMessage body];
                    if (![body isKindOfClass:[NSString class]]) {
                        return;
                    }

                    std::string msg = [body UTF8String];
                    this->_jscb(*this, msg);
                }
            }),
            "v@:@");

    WindowDelegate *delegate = [WindowDelegate alloc];
    [controller addScriptMessageHandler:delegate name:@"webview"];
    
    [_window setDelegate:delegate];

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    [_window setContentView:_webview];
    [_window makeKeyAndOrderFront:nil];

    setTitle(title);

    // Load index.html from the bundle
    NSURL* url = [[NSBundle mainBundle] URLForResource:@"index" withExtension: @"html"];
    [_webview loadFileURL:url allowingReadAccessToURL: [url URLByDeletingLastPathComponent]];
}

WebViewImpl::~WebViewImpl() 
{
}

bool WebViewImpl::run()
{
    NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:true];
    if (event) {
        [NSApp sendEvent:event];
    }

    return !_shouldExit;
}

void WebViewImpl::setTitle(const std::string& t)
{
    [_window setTitle:[NSString stringWithUTF8String:t.c_str()]];
}

void WebViewImpl::setFullscreen(bool fs)
{
    if (fs) {
        // TODO: replace toggle with set
        [_window toggleFullScreen:nil];
    } else {
        [_window toggleFullScreen:nil];
    }
}

void WebViewImpl::navigate(const std::string& u)
{
    [_webview loadRequest:
        [NSURLRequest requestWithURL:
            [NSURL URLWithString:
                [NSString stringWithUTF8String: u.c_str()]
            ]
        ]
    ];
}

void WebViewImpl::preEval(const std::string& js)
{
    _inject += "(()=>{" + js + "})()";
}

void WebViewImpl::eval(const std::string& js)
{
    [_webview evaluateJavaScript:[NSString stringWithUTF8String:js.c_str()] completionHandler:nil];
}

void WebViewImpl::css(const std::string& css)
{
    eval(R"js(
        (
            function (css)
            {
                if (document.styleSheets.length === 0) {
                    var s = document.createElement('style');
                    s.type = 'text/css';
                    document.head.appendChild(s);
                }
                document.styleSheets[0].insertRule(css);
            }
        )(')js" + css + "')");
}

void WebViewImpl::exit()
{
    // Distinguish window closing with app exiting
    _shouldExit = true;
    [NSApp terminate:nil];
}

Sim::WebView* Sim::WebView::create(int width, int height, bool resizable, bool debug, const std::string& title)
{
    return new WebViewImpl(width, height, resizable, debug, title);
}
