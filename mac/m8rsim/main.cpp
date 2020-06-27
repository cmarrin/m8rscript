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
static bool controlState = false;

static constexpr m8r::Duration ExecutionLoopIdleDelay = 2ms;
static constexpr m8r::Duration HeartOnTime = 100ms;

static void escape(std::string& s, char c, const char* replacement)
{
    size_t pos = 0;
    
    while (true) {
        pos = s.find(c, pos);
        if (pos == std::string::npos) {
            break;
        }
        s.replace(pos, 1, replacement);
        pos += strlen(replacement);
    }
}

static void escape(std::string& s)
{
    escape(s, '\n', "\\n");
    escape(s, '\'', "\\'");
}

int main(int argc, char **argv)
{
    // Create GUI WebPage
    Sim::WebView* wv = Sim::WebView::create(800, 600, true, true, "m8rScript Simulator");
    wv->setCallback([](Sim::WebView&, std::string& s)
    {
        m8r::Vector<m8r::String> v = m8r::String(s.c_str()).split(":");
        if (v.size() != 2) {
            return;
        }
        if (v[0] == "onkeydown") {
            if (v[1].size() == 1) {
                // This is a single char. Is it ^C
                if (v[1] == "c") {
                    application->receivedData("", m8r::KeyAction::Interrupt);
                } else if (application) {
                    application->receivedData(v[1], m8r::KeyAction::None);
                }
            } else {
                // Command
                m8r::KeyAction action = m8r::KeyAction::None;
                if (v[1] == "Enter") {
                    action = m8r::KeyAction::NewLine;
                } else if (v[1] == "Backspace") {
                    action = m8r::KeyAction::Backspace;
                } else if (v[1] == "Delete") {
                    action = m8r::KeyAction::Delete;
                } else if (v[1] == "ArrowUp") {
                    action = m8r::KeyAction::UpArrow;
                } else if (v[1] == "ArrowDown") {
                    action = m8r::KeyAction::DownArrow;
                } else if (v[1] == "ArrowRight") {
                    action = m8r::KeyAction::RightArrow;
                } else if (v[1] == "ArrowLeft") {
                    action = m8r::KeyAction::LeftArrow;
                } else if (v[1] == "Control") {
                    controlState = true;
                }
            }
        } else if (v[0] == "onkeyup") {
            // Record state of Control key so we can handle ^C
            if (v[1] == "Control") {
                controlState = false;
            }
        }
        
        ::printf("******** returned: %s\n", s.c_str());
    });
    
    std::mutex evalMutex;
    std::vector<std::string> evalArray;

    std::thread([wv, &evalMutex, &evalArray] {
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
            
            m8r::initMacSystemInterface(destFile.c_str(), [&evalMutex, &evalArray](const char* s) {
                std::lock_guard<std::mutex> lock(evalMutex);
                std::string ss(s);
                escape(ss);
                std::string line("document.getElementById('Console').value += '");
                line += ss;
                line += "';";
                evalArray.push_back(line);
            });
            application = new m8r::Application(23);
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
                std::string line("document.getElementById('LED1').className = 'led-bitmap led-");
                line += std::string((value & 0x04) ? "off" : "on") + "';";
                evalArray.push_back(line);
            }
            
            if (!busy) {
                usleep(static_cast<uint32_t>(ExecutionLoopIdleDelay.us()));
            }
        }
    }).detach();

    while (wv->run()) {
        std::vector<std::string> v;
        {
            std::lock_guard<std::mutex> lock(evalMutex);
            v = std::move(evalArray);
        }
        for (auto& it : v) {
            wv->eval(it);
        }
    }
    
    return 0;
}
