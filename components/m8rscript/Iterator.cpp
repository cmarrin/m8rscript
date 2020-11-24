/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Iterator.h"

#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8rscript;
using namespace m8r;

static StaticObject::StaticFunctionProperty _props[] =
{
    { SA::constructor, Iterator::constructor },
    { SA::done, Iterator::done },
    { SA::next, Iterator::next },
    { SA::getValue, Iterator::getValue },
    { SA::setValue, Iterator::setValue },
};

Iterator::Iterator()
{
    setProperties(_props, sizeof(_props) / sizeof(StaticFunctionProperty));
}

static bool done(ExecutionUnit* eu, Value thisValue, Mad<Object>& obj, int32_t& index)
{
    obj = thisValue.property(eu, SAtom(SA::__object)).asObject();
    index = thisValue.property(eu, SAtom(SA::__index)).asIntValue();
    if (!obj.valid()) {
        return true;
    }
    int32_t size = obj->property(SAtom(SA::length)).asIntValue();
    return index >= size;
}

CallReturnValue Iterator::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    Mad<Object> obj = eu->stack().top(1 - nparams).asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::InvalidArgumentValue);
    }
    
    thisValue.setProperty(SAtom(SA::__object), Value(obj), Value::SetType::AlwaysAdd);
    thisValue.setProperty(SAtom(SA::__index), Value(0), Value::SetType::AlwaysAdd);
    
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Iterator::done(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<Object> obj;
    int32_t index;
    eu->stack().push(Value(::done(eu, thisValue, obj, index)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Iterator::next(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<Object> obj;
    int32_t index;
    if (!::done(eu, thisValue, obj, index)) {
        ++index;
        if (!thisValue.setProperty(SAtom(SA::__index), Value(index), Value::SetType::NeverAdd)) {
            return CallReturnValue(Error::Code::InternalError);
        }
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Iterator::getValue(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<Object> obj;
    int32_t index;
    if (!::done(eu, thisValue, obj, index)) {
        eu->stack().push(obj->element(eu, Value(index)));
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Iterator::setValue(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    Mad<Object> obj;
    int32_t index;
    if (!::done(eu, thisValue, obj, index)) {
        obj->setElement(eu, Value(index), eu->stack().top(1 - nparams), Value::SetType::NeverAdd);
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
