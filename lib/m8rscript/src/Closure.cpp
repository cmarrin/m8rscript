/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Defines.h"
#ifndef M8RSCRIPT_SUPPORT
static_assert(0, "M8RSCRIPT_SUPPORT not defined");
#endif
#if M8RSCRIPT_SUPPORT == 1

#include "Closure.h"

#include "ExecutionUnit.h"

using namespace m8r;

Closure::~Closure()
{
    for (auto it : _upValues) {
        it.destroy(MemoryType::UpValue);
    }
}

void Closure::init(ExecutionUnit* eu, const Value& function, const Value& thisValue)
{
    _thisValue = thisValue;
    _func = function.asObject();
    assert(_func.valid());

    for (uint32_t i = 0; i < _func->upValueCount(); ++i) {
        Mad<UpValue> up = Mad<UpValue>::create(MemoryType::UpValue);

        uint32_t index;
        uint16_t frame;
        Atom name;
        if (!_func->upValue(i, index, frame, name)) {
            continue;
        }
        
        up->setStackIndex(eu->upValueStackIndex(index, frame - 1));
        eu->addOpenUpValue(up);
        _upValues.push_back(up);
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

CallReturnValue Closure::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (_thisValue) {
        thisValue = _thisValue;
    }
    eu->startFunction(Mad<Object>(this), thisValue.asObject(), nparams);
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

#endif
