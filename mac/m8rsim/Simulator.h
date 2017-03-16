/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#pragma once

#include "Application.h"
#include "ExecutionUnit.h"
#include "Shell.h"

class Simulator
{
public:
    Simulator(m8r::FS* fs)
        : _shell(fs, this)
    { }
    
    ~Simulator();
    
    void importBinary(const char* filename);
    void exportBinary(const char* filename);
    bool exportBinary(std::vector<uint8_t>&);
    const m8r::ErrorList* build(const char* name, bool debug);
    void printCode();
    void run();
    void pause();
    void stop();
    void simulate();
    void clear() { _shell.clear(); }
    
    bool isRunning() const { return _running; }

    bool canRun() { return true; }
    bool canStop() { return true; }
    bool canSaveBinary() { return _shell.program(); }
    
    void initShell() { _shell.init(); _receivedString.clear(); }
    long sendToShell(const void* data, long size);
    long receiveFromShell(void* data, long size);

    void shellSend(const char* data, uint16_t size)
    {
        if (!size) {
            size = strlen(data);
        }
        _receivedString += m8r::String(data, size);
    }

private:
    class MyShell : public m8r::Shell {
    public:
        MyShell(m8r::FS* fs, Simulator* simulator) : _application(fs), Shell(&_application), _simulator(simulator) { }
        virtual void shellSend(const char* data, uint16_t size = 0) override { _simulator->shellSend(data, size); }

    private:
        m8r::Application _application;
        Simulator* _simulator;
    };
    
    bool _running = false;
    MyShell _shell;
    m8r::String _receivedString;
};

