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
        using NextStates = Vector<std::pair<Input, State>>;
        
        struct StateEntry
        {
            StateEntry() { }
            
            StateEntry(State state, Action action, NextStates nextStates)
                : _state(state)
                , _action(action)
                , _nextStates(nextStates)
            { }
            
            StateEntry(State state, NextStates nextStates)
                : _state(state)
                , _nextStates(nextStates)
            { }
            
            StateEntry(State state, State jumpState, Action action = nullptr)
                : _state(state)
                , _action(action)
                , _jumpState(jumpState)
            { }
            
            State _state = State();
            Action _action = Action();
            NextStates _nextStates;
            State _jumpState = State();
        };
        
        StateMachine() { }
        StateMachine(const NextStates& nextStates) : _commonNextStates(nextStates) { }
        
        void addState(State state, Action action, const NextStates& nextStates)
        {
            _states.emplace_back(state, action, nextStates);
        }
        
        void addState(State state, const NextStates& nextStates)
        {
            _states.emplace_back(state, nextStates);
        }
        
        void addState(State state, State jumpState, Action action = nullptr)
        {
            _states.emplace_back(state, jumpState, action);
        }
        
        void gotoState(State state)
        {
            auto it = findState(state);
            if (it != _states.end()) {
                _currentState = state;
                
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
        using StateVector = Vector<StateEntry>;
        
        typename StateVector::const_iterator findState(State state)
        {
            return std::find_if(_states.begin(), _states.end(), [state](const StateEntry& entry) { return entry._state == state; });
        }
        
        StateVector _states;
        State _currentState = static_cast<State>(0);
        NextStates _commonNextStates;
    };

}
