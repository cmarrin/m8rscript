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

#include "Value.h"

using namespace m8r;

Telnet::Action Telnet::receive(char fromChannel, String& toChannel, String& toClient)
{
    // TODO: With the input set to raw, do we need to handle any IAC commands from Telnet?
    if (_state == State::Ready) {
        switch(fromChannel) {
            case '\xff': _state = State::ReceivedIAC; break;
            case '\e': _state = State::EscapeCSI; break;
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
                _position = 0;
                toChannel = "\r\n";
                break;
            case '\x7f': // backspace
                if (!_line.empty() && _position > 0) {
                    _position--;
                    _line.erase(_line.begin() + _position);

                    if (_position == _line.size()) {
                        // At the end of line. Do the simple thing
                        toChannel = "\x08 \x08";
                    } else {
                        toChannel = makeInputLine();
                    }
                }
                break;
            default:
                if (_position == _line.size()) {
                    // At end of line, do the simple thing
                    _line.push_back(fromChannel);
                    _position++;
                    toChannel = fromChannel;
                } else {
                    _line.insert(_line.begin() + _position, fromChannel);
                    _position++;
                    makeInputLine();
                }
                break;
        }
    } else if (_state == State::ReceivedIAC) {
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
    } else if (_state == State::EscapeCSI) {
        _state = (fromChannel == '[') ? State::EscapeParam : State::Ready;
    } else if (_state == State::EscapeParam) {
        if (fromChannel >= 0x20 && fromChannel <= 0x2f) {
            _escapeParam = fromChannel;
        } else {
            switch(fromChannel) {
                case 'D':
                    if (_position > 0) {
                        _position--;
                        toChannel = "\e[D";
                    }
                    break;
            }
            _state = State::Ready;
        }
    } else {
        _state = State::Ready;
    }
    
    return Action::None;
}

String Telnet::makeInputLine()
{
    String s = "\e[1000D\e[0K";
    s += String::join(_line);
    s += "\e[1000D";
    if (_position) {
        s += "\e[";
        
        // TODO: Need to move all this string processing stuff to String class
        s += Value::toString(_position);
        s += "C";
    }
    return s;
}
