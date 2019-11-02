//
//  StateMachine.h
//
//  Created by Chris Marrin on 3/25/2018
//
//

/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <algorithm>
#include <functional>
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
            StateEntry(State state, Action action, NextStates nextStates)
                : _state(state)
                , _action(action)
                , _nextStates(nextStates)
            { }
            
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
                
                if (_showStringCallback && it->_string.size()) {
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
                return;
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
        State _currentState = static_cast<State>(0);
        ShowStringCallback _showStringCallback;
        NextStates _commonNextStates;
    };

}
