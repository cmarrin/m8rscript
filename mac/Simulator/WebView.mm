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

Sim::WebView::WebView()
{
    wv::WebView w { 800, 600, true, true, "WebView", "http://www.google.com" };

    if (w.init() == -1) {
        return; // Error
    }

    while (w.run() == 0) { }
}

Sim::WebView::~WebView() 
{
}
