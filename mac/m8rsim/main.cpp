/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "WebView.h"
#include "Application.h"
#include "GPIOInterface.h"
#include "MacSystemInterface.h"

#include <fstream>
#include <streambuf>
#include <thread>
#include <mutex>
#include <unistd.h>

static m8r::Application* application = nullptr;

static constexpr m8r::Duration ExecutionLoopIdleDelay = 2ms;
static constexpr m8r::Duration HeartOnTime = 100ms;

static void escape(std::string& s)
{
    while (1) {
        size_t pos = s.find_first_of("\n");
        if (pos == std::string::npos) {
            return;
        }
        s.replace(pos, 1, "\\n");
    }
}

int main(int argc, char **argv)
{
    // Create GUI WebPage
    Sim::WebView* wv = Sim::WebView::create(800, 600, true, true, "m8rScript Simulator");

    std::mutex evalMutex;
    std::string evalString;

    std::thread([wv, &evalMutex, &evalString] {
        usleep(500000);

        // Init m8rscript
        if (!application) {
            // Copy the filesystem to the home directory so it's writable
            std::string fsFile = wv->pathForResource("m8rFSFile", "");
            std::string destFile = wv->homeDirectory() + "/m8rFSFile";
            
            if (!fsFile.empty()) {
                std::ifstream source(fsFile, std::ios::binary);
                
                if (!source.is_open()) {
                    ::printf("***** failed to open '%s': %s\n", fsFile.c_str(), strerror(errno));
                } else {
                    std::ofstream dest(destFile, std::ios::binary);
                    dest << source.rdbuf();
                    dest.close();
                }
                
                source.close();
            }
            
            m8r::initMacSystemInterface(destFile.c_str(), [&evalMutex, &evalString](const char* s) {
                std::lock_guard<std::mutex> lock(evalMutex);
                std::string ss(s);
                escape(ss);
                evalString += std::string("document.getElementById('Console').value += '");
                evalString += ss;
                evalString += "';";
            });
            application = new m8r::Application(800);
        }
        
        m8r::system()->setDefaultHeartOnTime(HeartOnTime);

        while (1) {
            bool busy = application->runOneIteration();
            
            uint32_t value = 0;
            uint32_t change = 0;
            if (m8r::system()->gpio()) {
                m8r::system()->gpio()->getState(value, change);
            }
            if ((change & 0x04) != 0) {
                // turn on LED if value & 0x04 is zero
                std::lock_guard<std::mutex> lock(evalMutex);
                evalString += std::string("document.getElementById('LED1').className = 'led-bitmap led-");
                evalString += std::string((value & 0x04) ? "off" : "on") + "';"; 
            }
            
            if (!busy) {
                usleep(static_cast<uint32_t>(ExecutionLoopIdleDelay.us()));
            }
        }
    }).detach();

    while (wv->run()) {
        std::string s;
        {
            std::lock_guard<std::mutex> lock(evalMutex);
            s = std::move(evalString);
        }
        if (!s.empty()) {
            wv->eval(s);
        }
    }
    
    return 0;
}
