/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Telnet.h"

using namespace m8r;

//StateTable<Telnet, Telnet::State, Telnet::Input> Telnet::_stateTable({ { { '\x01', '\xff'}, State::Ready } });

// This table can't be kept in ROM. It contains NextState vectors which get created on the fly from the
// initialization data. Those Vectors can't be in ROM. In this case we're only talking about 200 bytes.
StateTable<Telnet, Telnet::State, Telnet::Input>::StateEntry Telnet::_stateEntries[ ] = {
    { State::Ready,
        {
              { '\x7f', State::Backspace }
            , { { ' ', '\x7e' }, State::AddChar }
            , { '\x03', State::Interrupt }
            , { '\n', State::Ready }
            , { '\r', State::SendLine }
            , { '\e', State::CSI }
            , { '\xff', State::IAC }
        }
    },
    { State::Backspace, State::Ready, [](Telnet* telnet) { telnet->handleBackspace(); } },
    { State::AddChar, State::Ready, [](Telnet* telnet) { telnet->handleAddChar(); } },
    { State::Interrupt, State::Ready, [](Telnet* telnet) { telnet->handleInterrupt(); } },
    { State::SendLine, State::Ready, [](Telnet* telnet) { telnet->handleSendLine(); } },
    { State::CSI,
        {
            { '[', State::CSIBracket }
        }
    },
    { State::CSIBracket,
        {
              { { '\x30', '\x3f' }, State::CSIParam }
            , { { '\x40', '\x7e' }, State::CSICommand }
        }
    },
    { State::CSIParam, [](Telnet* telnet) { telnet->_csiParam = telnet->_currentChar; },
        {
              { { '\x40', '\x7e' }, State::CSICommand }
        }
    },
    { State::CSICommand, State::Ready, [](Telnet* telnet) { telnet->_csiParam = telnet->_currentChar; } },
    { State::IAC,
        {
                { { '\xf0', '\xfe' }, State::IACVerb }
              , { '\xff', State::AddFF }
        }
    },
    { State::AddFF, State::Ready, [](Telnet* telnet) { telnet->handleAddFF(); } },
    { State::IACVerb,
        {
              { { '\x01', '\x7e' }, State::IACCommand }
        }
    },
    { State::IACCommand, State::Ready, [](Telnet* telnet) { telnet->handleIACCommand(); } },
};

StateTable<Telnet, Telnet::State, Telnet::Input> Telnet::_stateTable(_stateEntries, sizeof(_stateEntries) / sizeof(StateTable<Telnet, Telnet::State, Telnet::Input>::StateEntry));

Telnet::Telnet()
    : _stateMachine(this, &_stateTable)
{
    reset();
}

void Telnet::handleBackspace()
{
    if (!_line.empty() && _position > 0) {
        _position--;
        _line.erase(_line.begin() + _position);

        if (_position == static_cast<int32_t>(_line.size())) {
            // At the end of line. Do the simple thing
            _toChannel = "\x08 \x08";
        } else {
            _toChannel = makeInputLine();
        }
    }    
}

void Telnet::handleAddChar()
{
    if (_position == static_cast<int32_t>(_line.size())) {
        // At end of line, do the simple thing
        _line.push_back(_currentChar);
        _position++;
        _toChannel = _currentChar;
    } else {
        _line.insert(_line.begin() + _position, _currentChar);
        _position++;
        _toChannel = makeInputLine();
    }
}

void Telnet::handleAddFF()
{
    _line.push_back('\xff');
}

void Telnet::handleInterrupt()
{
    _currentAction = KeyAction::Interrupt;
}

void Telnet::handleCSICommand()
{
    // TODO: Handle Params, etc.
    switch(_currentChar) {
        case 'D': // Cursor back
            if (_position > 0) {
                _position--;
                _toChannel = "\e[D";
            }
            break;
        case 'C': // Cursor forward
            if (_position < static_cast<int32_t>(_line.size()) - 1) {
                _position++;
                _toChannel = "\e[C";
            }
            break;
        case '~':
            switch(_csiParam) {
                case '3': // Delete forward
                    if (_position < static_cast<int32_t>(_line.size()) - 1) {
                        _line.erase(_line.begin() + _position);
                        _toChannel = makeInputLine();
                    }
                    break;
            }
    }
}

void Telnet::handleIACCommand()
{
    // TODO: Implement whatever is needed
}

void Telnet::handleSendLine()
{
    _toClient = String::join(_line);
    _line.clear();
    _position = 0;
    _toChannel = "\r\n";
    _currentAction = KeyAction::NewLine;
}

m8r::String Telnet::makeInputLine()
{
    String s = "\e[1000D\e[0K";
    s += String::join(_line);
    s += "\e[1000D";
    if (_position) {
        s += "\e[";
        
        // TODO: Need to move all this string processing stuff to String class
        s += String(_position);
        s += "C";
    }
    return s;
}

KeyAction Telnet::receive(char fromChannel, String& toChannel, String& toClient)
{
    _currentAction = KeyAction::None;
    _currentChar = fromChannel;
    _stateMachine.sendInput(_currentChar);
    
    std::swap(toChannel, _toChannel);
    std::swap(toClient, _toClient);
    return _currentAction;
}
