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

#include "Telnet.h"

using namespace m8r;

Telnet::Action Telnet::receive(char fromChannel, String& toChannel, String& toClient)
{
    // TODO: With the input set to raw, do we need to handle any IAC commands from Telnet?
    if (_state != State::Ready) {
        if (_state == State::ReceivedIAC) {
            switch(static_cast<Command>(fromChannel)) {
                case Command::DO: _verb = Verb::DO; break;
                case Command::DONT: _verb = Verb::DONT; break;
                case Command::WILL: _verb = Verb::WILL; break;
                case Command::WONT: _verb = Verb::WONT; break;
                case Command::IAC:
                    _line.push_back(fromChannel);
                    _state = State::Ready;
                    return Action::None;
                default: _verb = Verb::None; break;
            }
            _state = State::ReceivedVerb;
            return Action::None;
        } else if (_state == State::ReceivedVerb) {
            // TODO: Handle Verbs
            _state = State::Ready;
            return Action::None;
        }
    }
    
    switch(fromChannel) {
        case '\xff': _state = State::ReceivedIAC; break;
        case '\x03':
            // ctl-c
            toClient = fromChannel;
            break;
        case '\n':
            break; // Ignore newline
        case '\r':
            // We have a complete line
            toClient = String::join(_line);
            _line.clear();
            toChannel = "\r\n";
            break;
        default:
            _line.push_back(fromChannel);
            toChannel = fromChannel;
            break;
    }
    return Action::None;
}
