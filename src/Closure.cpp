/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Closure.h"

#include "ExecutionUnit.h"

using namespace m8r;

Closure::Closure(ExecutionUnit* eu, const Value& function, const Value& thisValue)
    : _thisValue(thisValue)
{
    assert(function.isFunction());
    _func = function.asObject();
    assert(_func);

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
    eu->startFunction(this, thisValue.asObject(), nparams, false);
    return CallReturnValue(CallReturnValue::Type::FunctionStart);
}

bool UpValue::closeIfNeeded(ExecutionUnit* eu, uint32_t frame)
{
    assert(!closed());
    if (stackIndex() >= frame) {
        value() = eu->stack().at(stackIndex());
        setClosed(true);
        return true;
    }
    return false;
}
