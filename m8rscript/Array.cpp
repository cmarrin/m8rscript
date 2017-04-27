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

#include "Array.h"

#include "ExecutionUnit.h"

using namespace m8r;

String Array::toString(ExecutionUnit* eu, bool typeOnly) const
{
    if (typeOnly) {
        return String("Array");
    }
    
    String s = "[ ";
    bool first = true;
    for (auto& it : _array) {
        if (first) {
            first = false;
        } else {
            s += ", ";
        }
        s += it.toStringValue(eu);
    }
    s += " ]";
    return s;
}

CallReturnValue Array::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    if (!ctor) {
        // FIXME: Do we want to handle calling an object as a function, like JavaScript does?
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    Array* array = new Array();
    eu->stack().push(Value(array));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

const Value Array::element(ExecutionUnit* eu, const Value& elt) const
{
    if (elt.isString()) {
        Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
        return property(eu, prop);
    }
    int32_t index = elt.toIntValue(eu);
    return (index >= 0 && index < _array.size()) ? _array[index] : Value();
}

bool Array::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
{
    if (append) {
        _array.push_back(value);
        return true;
    }
    
    if (elt.isString()) {
        Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
        return setProperty(eu, prop, value, Value::SetPropertyType::NeverAdd);
    }
    
    int32_t index = elt.toIntValue(eu);
    if (index < 0 || index >= _array.size()) {
        return false;
    }
    _array[index] = value;
    _needsGC = value.needsGC();
    return true;
}

const Value Array::property(ExecutionUnit*, const Atom& name) const
{
    return (name == ATOM(length)) ? Value(static_cast<int32_t>(_array.size())) : Value();
}

bool Array::setProperty(ExecutionUnit* eu, const Atom& name, const Value& value, Value::SetPropertyType type)
{
    if (type == Value::SetPropertyType::AlwaysAdd) {
        return false;
    }
    if (name == ATOM(length)) {
        _array.resize(value.toIntValue(eu));
        return true;
    }
    return false;
}
