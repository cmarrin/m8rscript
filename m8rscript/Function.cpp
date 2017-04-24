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

#include "Function.h"

#include "ExecutionUnit.h"

using namespace m8r;

Function::Function(Function* parent)
    : _parent(parent)
{
    addObject(this, true);

    // Place a dummy constant at index 0 as an error return value
    _constants.push_back(Value());
}

CallReturnValue Function::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    if (prop == ATOM(call)) {
        if (nparams < 1) {
            return CallReturnValue(CallReturnValue::Type::Error);
        }
        
        // Remove the first element and use it as the this pointer
        Value self = eu->stack().top(1 - nparams);
        eu->stack().remove(1 - nparams);
        nparams--;
    
        return call(eu, self, nparams, false);
    }
    return CallReturnValue(CallReturnValue::Type::Error);
}

CallReturnValue Function::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    eu->startFunction(objectId(), thisValue.asObjectId(), nparams, _parent == eu->currentFunction());
    return CallReturnValue(CallReturnValue::Type::FunctionStart);
}

int32_t Function::addLocal(const Atom& atom)
{
    for (auto name : _locals) {
        if (name == atom) {
            return -1;
        }
    }
    _locals.push_back(atom);
    return static_cast<int32_t>(_locals.size()) - 1;
}

int32_t Function::localIndex(const Atom& name) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(_locals.size()); ++i) {
        if (_locals[i] == name) {
            return i;
        }
    }
    return -1;
}

ConstantId Function::addConstant(const Value& v)
{
    assert(_constants.size() < std::numeric_limits<uint8_t>::max());
    
    for (ConstantId::value_type id = 0; id < _constants.size(); ++id) {
        if (_constants[id] == v) {
            return ConstantId(id);
        }
    }
    
    ConstantId r(static_cast<ConstantId::value_type>(_constants.size()));
    _constants.push_back(v);
    return r;
}

uint32_t Function::addUpValue(uint32_t index, uint16_t frame, Atom name)
{
    assert(_upValues.size() < std::numeric_limits<uint16_t>::max());
    UpValueEntry entry(index, frame, name);
    
    for (uint32_t i = 0; i < _upValues.size(); ++i) {
        if (_upValues[i] == entry) {
            return i;
        }
            
    }
    _upValues.push_back(entry);
    return static_cast<uint32_t>(_upValues.size()) - 1;
}

uint32_t Function::upValueStackIndex(ExecutionUnit* eu, uint32_t index) const
{
    return eu->upValueStackIndex(_upValues[index]._index, _upValues[index]._frame);
}
