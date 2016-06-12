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

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "Value.h"

namespace m8r {

class Object;

typedef union {
    void* v;
    float f;
    int32_t i;
    Object* o;
    const char* s;
    Atom a;
} U;

inline void* valueFromFloat(float f) { U u; u.f = f; return u.v; }
inline void* valueFromInt(int32_t i) { U u; u.i = i; return u.v; }
inline void* valueFromObj(Object* o) { U u; u.o = o; return u.v; }
inline void* valueFromStr(const char* s) { U u; u.s = s; return u.v; }
inline void* valueFromId(Atom a) { U u; u.a = a; return u.v; }

inline float floatFromValue(void* v) { U u; u.v = v; return u.f; }
inline int32_t intFromValue(void* v) { U u; u.v = v; return u.i; }
inline Object* objFromValue(void* v) { U u; u.v = v; return u.o; }
inline const char* strFromValue(void* v) { U u; u.v = v; return u.s; }
inline Atom idFromValue(void* v) { U u; u.v = v; return u.a; }

class Value {
public:
    typedef m8r::Map<Atom, Value> Map;
    enum class Type { None, Object, Float, Integer, String, Id };

    Value() : _value(nullptr), _type(Type::None) { }
    Value(const Value& other) : _value(other._value), _type(other._type) { }
    
    Value(Object* obj) : _value(valueFromObj(obj)) , _type(Type::Object) { }
    Value(float value) : _value(valueFromFloat(value)) , _type(Type::Float) { }
    Value(int32_t value) : _value(valueFromInt(value)) , _type(Type::Integer) { }
    Value(const char* value) : _value(valueFromStr(value)) , _type(Type::String) { }
    Value(Atom value) : _value(valueFromId(value)), _type(Type::Id) { }
    
    ~Value();
    
    Type type() const { return _type; }
    Object* objectValue() const { return (_type == Type::Object) ? objFromValue(_value) : nullptr; }
    bool boolValue() const;
    int32_t intValue() const { return (_type == Type::Integer) ? intFromValue(_value) : 0; }
    float floatValue() const { return (_type == Type::Float) ? floatFromValue(_value) : 0; }
    const char* stringValue() const { return (_type == Type::String) ? strFromValue(_value) : nullptr; }
    Atom atomValue() const { return (_type == Type::Id) ? idFromValue(_value) : Atom::emptyAtom(); }
    
private:
    void* _value;
    Type _type;
};

}
