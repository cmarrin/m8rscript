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

#include "Shell.h"

using namespace esp;

void Shell::connected()
{
    _state = State::Init;
    sendComplete();
}

bool Shell::received(const char* data, uint16_t size)
{
    m8r::Vector<m8r::String> array = m8r::String(data).trim().split(" ", true);
    return executeCommand(array);
}

void Shell::sendComplete()
{
    switch(_state) {
        case State::Init:
            _output->shellSend("\nWelcome to m8rscript\n");
            _state = State::NeedPrompt;
            break;
        case State::NeedPrompt:
            _output->shellSend("\n> ");
            _state = State::ShowingPrompt;
            break;
        case State::ShowingPrompt:
            break;
        case State::ListFiles:
            if (_directoryEntry && _directoryEntry->valid()) {
                char buf[60];
                os_sprintf(buf, "File:%s:%d\n", _directoryEntry->name(), _directoryEntry->size());
                _output->shellSend(buf);
                _directoryEntry->next();
            } else {
                if (_directoryEntry) {
                    delete _directoryEntry;
                    _directoryEntry = nullptr;
                }
                _state = State::NeedPrompt;
                sendComplete();
            }
            break;
    }
}

bool Shell::executeCommand(const m8r::Vector<m8r::String>& array)
{
    if (array.size() == 0) {
        return true;
    }
    if (array[0] == "ls") {
        _directoryEntry = esp::FS::sharedFS()->directory();
        _state = State::ListFiles;
        sendComplete();
    } else if (array[0] == "t") {
    } else if (array[0] == "b") {
    } else if (array[0] == "get") {
    } else if (array[0] == "put") {
    } else if (array[0] == "quit") {
        return false;
    }
    return true;
}
