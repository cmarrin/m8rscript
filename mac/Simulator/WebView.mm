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

Sim::WebView::WebView(int width, int height, const std::string& title, const std::string& html)
{
    wv::WebView* w = new wv::WebView(width, height, true, true, title.c_str(), html.c_str());
    _webView = w;

    if (w->init() == -1) {
        delete w;
        _webView = nullptr;
        return;
    }
}

Sim::WebView::~WebView() 
{
}

bool Sim::WebView::run()
{
    return webView(_webView)->run();
}
