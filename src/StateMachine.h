//
//  StateMachine.h
//
//  Created by Chris Marrin on 3/25/2018
//
//

/*
Copyright (c) 2009-2018 Chris Marrin (chris@marrin.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of Marrinator nor the names of its contributors may be
      used to endorse or promote products derived from this software without
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

#pragma once

#include <m8r.h>
#include <vector>

namespace m8r {

    // StateMachine class
    //
    // Store a state machine structure with the given States and traverse using the given Inputs.
    template<typename State, typename Input>
    class StateMachine
    {
    public:
        using Action = std::function<void()>;
        using NextStates = std::vector<std::pair<Input, State>>;
        using ShowStringCallback = std::function<void(const String&)>;
        
        struct StateEntry
        {
            StateEntry(State state, Action action, NextStates nextStates, const String& s)
                : _state(state)
                , _action(action)
                , _nextStates(nextStates)
                , _string(s)
            { }
            
            StateEntry(State state, Action action, State jumpState, const String& s)
                : _state(state)
                , _action(action)
                , _jumpState(jumpState)
                , _string(s)
            { }
            
            State _state;
            Action _action;
            NextStates _nextStates;
            State _jumpState;
            String _string;
        };
        
        StateMachine() { }
        StateMachine(ShowStringCallback cb) : _showStringCallback(cb) { }
        StateMachine(const NextStates& nextStates) : _commonNextStates(nextStates) { }
        StateMachine(ShowStringCallback cb, const NextStates& nextStates) : _showStringCallback(cb) , _commonNextStates(nextStates) { }
        
        void addState(State state, Action action, const NextStates& nextStates)
        {
            _states.emplace_back(state, action, nextStates, "");
        }
        
        void addState(State state, const String& s, const NextStates& nextStates)
        {
            _states.emplace_back(state, nullptr, nextStates, s);
        }
        
        void addState(State state, const String& s, Action action, const NextStates& nextStates)
        {
            _states.emplace_back(state, action, nextStates, s);
        }
        
        void addState(State state, Action action, State jumpState)
        {
            _states.emplace_back(state, action, jumpState, "");
        }
        
        void addState(State state, const String& s, State jumpState)
        {
            _states.emplace_back(state, nullptr, jumpState, s);
        }
        
        void addState(State state, const String& s, Action action, State jumpState)
        {
            _states.emplace_back(state, action, jumpState, s);
        }
        
        void gotoState(State state)
        {
            auto it = findState(state);
            if (it != _states.end()) {
                _currentState = state;
                
                if (_showStringCallback && it->_string.length()) {
                    _showStringCallback(it->_string);
                }
                
                if (it->_action) {
                    it->_action();
                }
                
                if (it->_nextStates.empty()) {
                    gotoState(it->_jumpState);
                }
            }
        }
        
        void sendInput(Input input)
        {
            // Check next states for currentState
            auto it = findState(_currentState);
            if (it == _states.end()) {
                return;
            }

            auto inputIt = std::find_if(it->_nextStates.begin(), it->_nextStates.end(), [input](const std::pair<Input, State>& entry) {
                return entry.first == input;
            });

            if (inputIt != it->_nextStates.end()) {
                gotoState(inputIt->second);
            }

            // Check the common next states
            auto commonIt = std::find_if(_commonNextStates.begin(), _commonNextStates.end(), [input](const std::pair<Input, State>& entry) {
                return entry.first == input;
            });
            if (commonIt != _commonNextStates.end()) {
                gotoState(commonIt->second);
            }
        }
        
    private:
        using StateVector = std::vector<StateEntry>;
        
        typename StateVector::const_iterator findState(State state)
        {
            return std::find_if(_states.begin(), _states.end(), [state](const StateEntry& entry) { return entry._state == state; });
        }
        
        StateVector _states;
        State _currentState;
        ShowStringCallback _showStringCallback;
        NextStates _commonNextStates;
    };

}
