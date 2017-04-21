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

#include "Closure.h"

#include "ExecutionUnit.h"

using namespace m8r;

Closure::Closure(ExecutionUnit* eu, const Value& function, const Value& thisValue)
    : _thisValue(thisValue)
{
    assert(function.isFunction());
    _func = reinterpret_cast<Function*>(static_cast<Object*>(function.asObjectId()));
    assert(_func);
    addObject(this, true);

    for (uint32_t i = 0; i < _func->upValueCount(); ++i) {
        _upValues.push_back(eu->newUpValue(_func->upValueStackIndex(eu, i)));
    }
}

bool Closure::loadUpValue(ExecutionUnit* eu, uint32_t index, Value& value) const
{
    assert(index < _upValues.size() && _upValues.size() == _func->upValueCount());
    if (_upValues[index]->closed()) {
        value = _upValues[index]->value();
    } else {
        value = eu->stack().at(_upValues[index]->stackIndex());
    }
    return true;
}

bool Closure::storeUpValue(ExecutionUnit* eu, uint32_t index, const Value& value)
{
    assert(index < _upValues.size() && _upValues.size() == _func->upValueCount());
    if (_upValues[index]->closed()) {
        _upValues[index]->value() = value;
    } else {
        eu->stack().at(_upValues[index]->stackIndex()) = value;
    }
    return true;
}

void Closure::closeUpValues(ExecutionUnit* eu, uint32_t frame)
{
    for (auto& it : _upValues) {
        if (!it->closed() && it->stackIndex() >= frame) {
            it->value() = eu->stack().at(it->stackIndex());
            it->setClosed(true);
        }
    }
}

CallReturnValue Closure::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    if (!thisValue) {
        thisValue = _thisValue;
    }
    eu->startFunction(objectId(), thisValue.asObjectId(), nparams, false);
    return CallReturnValue(CallReturnValue::Type::FunctionStart);
}

