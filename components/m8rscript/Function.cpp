/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Function.h"

#include "ExecutionUnit.h"

using namespace m8rscript;
using namespace m8r;

Function::Function()
{
}

bool Function::builtinConstant(uint8_t reg, Value& value)
{
    if (reg <= MaxRegister) {
        // Should never get here
        assert(0);
        return false;
    }
    
    reg -= MaxRegister + 1;
    
    switch(static_cast<BuiltinConstants>(reg)) {
        case BuiltinConstants::Undefined: value = Value(); return true;
        case BuiltinConstants::Null: value = Value::NullValue(); return true;
        case BuiltinConstants::Int0: value = Value(0); return true;
        case BuiltinConstants::Int1: value = Value(1); return true;
        case BuiltinConstants::AtomShort:
        case BuiltinConstants::AtomLong: assert(0); return false; // Handled outside, should never get here
        default: return false;
    }
}

CallReturnValue Function::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    if (prop == SAtom(SA::call)) {
        if (nparams < 1) {
            return CallReturnValue(Error::Code::WrongNumberOfParams);
        }
        
        // Remove the first element and use it as the this pointer
        Value self = eu->stack().top(1 - nparams);
        eu->stack().remove(1 - nparams);
        nparams--;
    
        return call(eu, self, nparams);
    }
    return CallReturnValue(Error::Code::PropertyDoesNotExist);
}

CallReturnValue Function::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    eu->startFunction(Mad<Function>(this), thisValue.asObject(), nparams);
    return CallReturnValue(CallReturnValue::Type::FunctionStart);
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

bool Function::loadUpValue(ExecutionUnit* eu, uint32_t index, Value& value) const
{
    assert(index < _upValues.size());
    value = eu->stack().at(eu->upValueStackIndex(_upValues[index]._index, _upValues[index]._frame));
    return true;
}
