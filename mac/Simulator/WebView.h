/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <string>

// This is the interface to webview_impl

namespace Sim {

class WebView
{
public:
    WebView(int width, int height, const std::string& title, const std::string& html);
    ~WebView();
    
    bool run();
    
private:
    void* _webView = nullptr;
};

}
