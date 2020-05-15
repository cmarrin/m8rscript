#include <functional>
#include <string>
#import <Cocoa/Cocoa.h>
#import <Webkit/Webkit.h>
#include <objc/objc-runtime.h>

// ObjC declarations may only appear in global scope
@interface WindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
@end

@implementation WindowDelegate
- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)scriptMessage {
}
@end

constexpr auto DEFAULT_URL =
    "data:text/"
    "html,%3C%21DOCTYPE%20html%3E%0A%3Chtml%20lang=%22en%22%3E%0A%3Chead%3E%"
    "3Cmeta%20charset=%22utf-8%22%3E%3Cmeta%20http-equiv=%22X-UA-Compatible%22%"
    "20content=%22IE=edge%22%3E%3C%2Fhead%3E%0A%3Cbody%3E%3Cdiv%20id=%22app%22%"
    "3E%3C%2Fdiv%3E%3Cscript%20type=%22text%2Fjavascript%22%3E%3C%2Fscript%3E%"
    "3C%2Fbody%3E%0A%3C%2Fhtml%3E";

/*
<!DOCTYPE html>
<html lang="en">
<head><meta charset="utf-8"><meta http-equiv="X-UA-Compatible"
content="IE=edge"></head> <body><div id="app"></div><script
type="text/javascript"></script></body>
</html>
*/

namespace wv {
using String = std::string;

class WebView {
  using jscb = std::function<void(WebView &, std::string &)>;

public:
  WebView(int width, int height, bool resizable, bool debug, String title,
          String url = DEFAULT_URL)
      : width(width), height(height), resizable(resizable), debug(debug),
        title(title), url(url) {}
  int init();                      // Initialize webview
  void setCallback(jscb callback); // JS callback
  void setTitle(String t);         // Set title of window
  void setFullscreen(bool fs);     // Set fullscreen
  void setBgColor(uint8_t r, uint8_t g, uint8_t b,
                  uint8_t a); // Set background color
  bool run();                 // Main loop
  void navigate(String u);    // Navigate to URL
  void preEval(String js);    // Eval JS before page loads
  void eval(String js);       // Eval JS
  void css(String css);       // Inject CSS
  void exit();                // Stop loop

private:
  // Properties for init
  int width;
  int height;
  bool resizable;
  bool fullscreen = false;
  bool debug;
  String title;
  String url;

  jscb js_callback;
  bool init_done = false; // Finished running init
  uint8_t bgR = 255, bgG = 255, bgB = 255, bgA = 255;

  String inject = "window.external={invoke:arg=>window.webkit.messageHandlers.webview.postMessage(arg)};";
  bool should_exit = false; // Close window
  //NSAutoreleasePool *pool;
  NSWindow *window;
  WKWebView *webview;
};

int WebView::init() {
  // Initialize autorelease pool
  //pool = [NSAutoreleasePool new];

  // Window style: titled, closable, minimizable
  uint style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
               NSWindowStyleMaskMiniaturizable;

  // Set window to be resizable
  if (resizable) {
    style |= NSWindowStyleMaskResizable;
  }

  // Initialize Cocoa window
  window = [[NSWindow alloc]
      // Initial window size
      initWithContentRect:NSMakeRect(0, 0, width, height)
                // Window style
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
      imp_implementationWithBlock(
          [=](id self, SEL cmd, id notification) { this->exit(); }),
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

  setTitle(title);
  if (fullscreen) {
    setFullscreen(true);
  }
  setBgColor(bgR, bgG, bgB, bgA);
  navigate(url);

  return 0;
}

void WebView::setCallback(jscb callback) { js_callback = callback; }

void WebView::setTitle(std::string t) {
  if (!init_done) {
    title = t;
  } else {
    [window setTitle:[NSString stringWithUTF8String:t.c_str()]];
  }
}

void WebView::setFullscreen(bool fs) {
  if (!init_done) {
    fullscreen = fs;
  } else if (fs) {
    // TODO: replace toggle with set
    [window toggleFullScreen:nil];
  } else {
    [window toggleFullScreen:nil];
  }
}

void WebView::setBgColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (!init_done) {
    bgR = r;
    bgG = g;
    bgB = b;
    bgA = a;
  } else {
    [window setBackgroundColor:[NSColor colorWithCalibratedRed:r / 255.0
                                                         green:g / 255.0
                                                          blue:b / 255.0
                                                         alpha:a / 255.0]];
  }
}

bool WebView::run() {
  NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate distantFuture]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:true];
  if (event) {
    [NSApp sendEvent:event];
  }

  return should_exit;
}

void WebView::navigate(std::string u) {
  if (!init_done) {
    url = u;
  } else {
    [webview
        loadRequest:[NSURLRequest
                        requestWithURL:
                            [NSURL URLWithString:[NSString stringWithUTF8String:
                                                               u.c_str()]]]];
  }
}

void WebView::preEval(std::string js) { inject += "(()=>{" + js + "})()"; }

void WebView::eval(std::string js) {
  [webview evaluateJavaScript:[NSString stringWithUTF8String:js.c_str()]
            completionHandler:nil];
}

void WebView::css(std::string css) {
  eval(R"js(
    (
      function (css) {
        if (document.styleSheets.length === 0) {
          var s = document.createElement('style');
          s.type = 'text/css';
          document.head.appendChild(s);
        }
        document.styleSheets[0].insertRule(css);
      }
    )(')js" +
       css + "')");
}

void WebView::exit() {
  // Distinguish window closing with app exiting
  should_exit = true;
  [NSApp terminate:nil];
}

} // namespace wv
