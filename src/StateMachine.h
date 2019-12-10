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
        using NextState = std::pair<Input, State>;
        using NextStates = Vector<NextState>;
        
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
            uint16_t _nextStateCount = 0;
            uint16_t _nextStateIndex = 0;
        };
        
        StateTable(StateEntry* states, uint32_t numStates) : _states(states), _numStates(numStates) { collapseStates(); }
        StateTable(StateEntry* states, uint32_t numStates, const NextStates& nextStates) : _states(states), _numStates(numStates), _commonNextStates(nextStates) {collapseStates(); }
        
        bool empty() const { return _numStates == 0; }
        
        void gotoNextState(State state, State& next, T* owner)
        {
            auto it = findState(state);
            if (it != _states + _numStates) {
                next = state;
                
                if (it->_action) {
                    it->_action(owner);
                }
                
                if (it->_nextStateCount == 0) {
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

            NextState* nextStates = &(_nextStateBuffer.get()[it->_nextStateIndex]);
            auto inputIt = std::find_if(nextStates, nextStates + it->_nextStateCount, [input](const std::pair<Input, State>& entry) {
                return entry.first == input;
            });

            if (inputIt != nextStates + it->_nextStateCount) {
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
        void collapseStates()
        {
            // Put all NextStates vectors in a single buffer
            // First see how big the buffer needs to be
            uint32_t count = 0;
            for (uint32_t i = 0; i < _numStates; ++i) {
                count += _states[i]._nextStates.size();
            }
            
            _nextStateBuffer = Mad<NextState>::create(MemoryType::Vector, count);
            
            // Fill in the buffer
            uint32_t index = 0;

            for (uint32_t i = 0; i < _numStates; ++i) {
                count = static_cast<uint32_t>(_states[i]._nextStates.size());
                _states[i]._nextStateCount = count;
                _states[i]._nextStateIndex = index;
                
                for (auto it : _states[i]._nextStates) {
                    _nextStateBuffer.get()[index++] = it;
                }
                _states[i]._nextStates = NextStates();
            }
        }
        
        const StateEntry* findState(State state)
        {
            return std::find_if(_states, _states + _numStates, [state](const StateEntry& entry) { return entry._state == state; });
        }
        
        StateEntry* _states = nullptr;
        uint32_t _numStates = 0;
        NextStates _commonNextStates;
        Mad<NextState> _nextStateBuffer;
    };


    // StateMachine class
    //
    // Store a state machine structure with the given States and traverse using the given Inputs.
    template<typename T, typename State, typename Input>
    class StateMachine
    {
    public:
        StateMachine(T* owner, StateTable<T, State, Input>* table) : _owner(owner), _table(table) { }
        
        void gotoState(State state) { _table->gotoNextState(state, _currentState, _owner); }
        
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
