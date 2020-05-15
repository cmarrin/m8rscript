/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "WebView.h"

#include <fstream>
#include <streambuf>
#include <unistd.h>

int main(int argc, char **argv)
{
    // Load the index.html file
    chdir("../../Simulator");
    char cwd[PATH_MAX];
    printf("cwd='%s'\n", getwd(cwd));
    std::ifstream t("index.html");
    
    if (!t.is_open()) {
        printf("***** failed to open index.html: %s\n", strerror(errno));
        return -1;
    }
    
    std::string html;
    
    t.seekg(0, std::ios::end);
    html.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    
    html.assign((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());
    Sim::WebView wv(800, 600, "m8rScript Simulator", html);
    
    while (wv.run()) { }
    return 0;
}
