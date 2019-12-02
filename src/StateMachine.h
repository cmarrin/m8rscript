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

    // StateTable class
    //
    // Stores the state transitions. The table is stateless. It is used by StateMachine below to do all the transitions
    // so it can be shared among several state machines.
    template<typename T, typename State, typename Input>
    class StateTable
    {
    public:
        using Action = void(*)(T*);
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
        
        StateTable(StateEntry* states, uint32_t numStates) : _states(states), _numStates(numStates) { }
        StateTable(StateEntry* states, uint32_t numStates, const NextStates& nextStates) : _states(states), _numStates(numStates), _commonNextStates(nextStates) { }
        
        bool empty() const { return _numStates == 0; }
        
        void nextState(State state, State& next, T* owner)
        {
            auto it = findState(state);
            if (it != _states + _numStates) {
                next = state;
                
                if (it->_action) {
                    it->_action(owner);
                }
                
                if (it->_nextStates.empty()) {
                    next = it->_jumpState;
                }
            }
        }

        bool sendInput(Input input, State currentState, State& nextState)
        {
            // Check next states for currentState
            auto it = findState(currentState);
            if (it == _states + _numStates) {
                return false;
            }

            auto inputIt = std::find_if(it->_nextStates.begin(), it->_nextStates.end(), [input](const std::pair<Input, State>& entry) {
                return entry.first == input;
            });

            if (inputIt != it->_nextStates.end()) {
                nextState = inputIt->second;
                return true;
            }

            // Check the common next states
            auto commonIt = std::find_if(_commonNextStates.begin(), _commonNextStates.end(), [input](const std::pair<Input, State>& entry) {
                return entry.first == input;
            });
            if (commonIt != _commonNextStates.end()) {
                nextState = commonIt->second;
                return true;
            }
            return false;
        }
        
    private:
        using StateVector = Vector<StateEntry>;
        
        typename StateVector::const_iterator findState(State state)
        {
            return std::find_if(_states, _states + _numStates, [state](const StateEntry& entry) { return entry._state == state; });
        }
        
        StateEntry* _states = nullptr;
        uint32_t _numStates = 0;
        NextStates _commonNextStates;
    };


    // StateMachine class
    //
    // Store a state machine structure with the given States and traverse using the given Inputs.
    template<typename T, typename State, typename Input>
    class StateMachine
    {
    public:
        StateMachine(T* owner, StateTable<T, State, Input>* table) : _owner(owner), _table(table) { }
        
        void gotoState(State state) { _table->nextState(state, _currentState, _owner); }
        
        void sendInput(Input input)
        {
            State nextState;
            if (_table->sendInput(input, _currentState, nextState)) {
                gotoState(nextState);
            }
        }
        
    private:
        T* _owner = nullptr;
        StateTable<T, State, Input>* _table;
        State _currentState = static_cast<State>(0);
    };

}
