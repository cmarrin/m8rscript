/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <string>

namespace Sim {

class WebView
{
public:
    using jscb = std::function<void(WebView &, std::string &)>;

    static WebView* create(int width, int height, bool resizable, bool debug, const std::string& title);
    
    virtual ~WebView() { }
    
    void setCallback(jscb callback) { _jscb = callback; }
    
    virtual bool run() = 0;
    
    virtual void setTitle(const std::string& t) = 0;
    virtual void setFullscreen(bool fs) = 0;
    virtual void navigate(const std::string& u) = 0;
    virtual void preEval(const std::string& js) = 0;
    virtual void eval(const std::string& js) = 0;
    virtual void css(const std::string& css) = 0;
    virtual void exit() = 0;

protected:
    bool _resizable = true;
    bool _debug = false;

    jscb _jscb;

    std::string _inject = "window.external={invoke:arg=>window.webkit.messageHandlers.webview.postMessage(arg)};";
    bool _shouldExit = false;
};

}
