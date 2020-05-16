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
#include <unistd.h>

static m8r::Application* application = nullptr;

int main(int argc, char **argv)
{
    // Create GUI WebPage
    Sim::WebView* wv = Sim::WebView::create(800, 600, false, true, "m8rScript Simulator");

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
        
        m8r::initMacSystemInterface(destFile.c_str(), [](const char* s) {
            //consoleString += s;
            ::printf("%s", s);
        });
        application = new m8r::Application(800);
    }

    while (wv->run()) {
        application->runOneIteration();
        
        uint32_t value, change;
        m8r::system()->gpio()->getState(value, change);
        if ((change & 0x04) != 0) {
            // turn on LED if value & 0x04 is not zero
        }
    }
    
    delete wv;
    return 0;
}







#if 0


class ViewController: NSViewController {

    @IBOutlet weak var LED1: NSButton!
    @IBOutlet var Console: NSTextView!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        LED1.state = NSControl.StateValue.on

        
        Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { timer in
            m8rRunOneIteration()
            
            var value: UInt32 = 0
            var change: UInt32 = 0
            m8rGetGPIOState(&value, &change)
            if ((change & 0x04) != 0) {
                self.LED1.state = ((value & 0x04) != 0) ? NSControl.StateValue.off : NSControl.StateValue.on
            }
            
            self.Console.textStorage?.append(NSAttributedString(string: String(cString:m8rGetConsoleString())));
        }
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }






static m8r::Application* application = nullptr;

static m8r::String consoleString; 

const char* m8rGetConsoleString()
{
    m8r::String s = std::move(consoleString);
    return s.c_str();
}

#endif
