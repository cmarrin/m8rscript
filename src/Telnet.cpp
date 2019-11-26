/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Telnet.h"

#include "Value.h"

using namespace m8r;

StateTable<Telnet, Telnet::State, Telnet::Input> Telnet::_stateTable({ { { '\x01', '\xff'}, State::Ready } });

Telnet::Telnet()
    : _stateMachine(this, &_stateTable)
{
    if (_stateTable.empty()) {
        _stateTable.addState(State::Ready,
        {
              { '\x7f', State::Backspace }
            , { { ' ', '\x7e' }, State::AddChar }
            , { '\x03', State::Interrupt }
            , { '\n', State::Ready }
            , { '\r', State::SendLine }
            , { '\e', State::CSI }
            , { '\xff', State::IAC }
        });
        
        _stateTable.addState(State::Backspace, State::Ready, [](Telnet* telnet) { telnet->handleBackspace(); });
        _stateTable.addState(State::AddChar, State::Ready, [](Telnet* telnet) { telnet->handleAddChar(); });
        _stateTable.addState(State::Interrupt, State::Ready, [](Telnet* telnet) { telnet->handleInterrupt(); });
        _stateTable.addState(State::SendLine, State::Ready, [](Telnet* telnet) { telnet->handleSendLine(); });

        // CSI
        _stateTable.addState(State::CSI,
        {
             { '[', State::CSIBracket }
        });

        _stateTable.addState(State::CSIBracket,
        {
              { { '\x30', '\x3f' }, State::CSIParam }
            , { { '\x40', '\x7e' }, State::CSICommand }
        });

        _stateTable.addState(State::CSIParam, [](Telnet* telnet) { telnet->_csiParam = telnet->_currentChar; },
        {
               { { '\x40', '\x7e' }, State::CSICommand }
        });

        _stateTable.addState(State::CSICommand, State::Ready, [](Telnet* telnet) { telnet->handleCSICommand(); });

        // IAC
        _stateTable.addState(State::IAC,
        {
             { { '\xf0', '\xfe' }, State::IACVerb }
           , { '\xff', State::AddFF }
        });

        _stateTable.addState(State::AddFF, State::Ready, [](Telnet* telnet) { telnet->handleAddFF(); });

        _stateTable.addState(State::IACVerb,
        {
              { { '\x01', '\x7e' }, State::IACCommand }
        });

        _stateTable.addState(State::IACCommand, State::Ready, [](Telnet* telnet) { telnet->handleIACCommand(); });
    }
}

void Telnet::handleBackspace()
{
    if (!_line.empty() && _position > 0) {
        _position--;
        _line.erase(_line.begin() + _position);

        if (_position == _line.size()) {
            // At the end of line. Do the simple thing
            _toChannel = "\x08 \x08";
        } else {
            _toChannel = makeInputLine();
        }
    }    
}

void Telnet::handleAddChar()
{
    if (_position == _line.size()) {
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
    _currentAction = Action::Interrupt;
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
            if (_position < _line.size() - 1) {
                _position++;
                _toChannel = "\e[C";
            }
            break;
        case '~':
            switch(_csiParam) {
                case '3': // Delete forward
                    if (_position < _line.size() - 1) {
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
    _currentAction = Action::NewLine;
}

String Telnet::makeInputLine()
{
    String s = "\e[1000D\e[0K";
    s += String::join(_line);
    s += "\e[1000D";
    if (_position) {
        s += "\e[";
        
        // TODO: Need to move all this string processing stuff to String class
        s += String::toString(_position);
        s += "C";
    }
    return s;
}

Telnet::Action Telnet::receive(char fromChannel, String& toChannel, String& toClient)
{
    _currentAction = Action::None;
    _currentChar = fromChannel;
    _stateMachine.sendInput(_currentChar);
    
    std::swap(toChannel, _toChannel);
    std::swap(toClient, _toClient);
    return _currentAction;
}
