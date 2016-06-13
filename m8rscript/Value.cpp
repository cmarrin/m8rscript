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

#include "Value.h"

#include "Object.h"

using namespace m8r;

Value::~Value()
{
}

bool Value::boolValue() const
{
    switch(_type) {
        case Type::None: return false;
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = objectValue();
            if (obj) {
                v = obj->value();
            }
            return v ? v->boolValue() : false;
        }
        case Type::Float: return floatValue() != 0;
        case Type::Integer: return intValue() != 0;
        case Type::String: {
            const char* s = stringValue();
            return s ? (s[0] != '\0') : false;
        }
        case Type::Id: return false;
        case Type::Ref:
            return objFromValue()->property(_id)->boolValue();
    }
}

bool Value::setValue(const Value& v)
{
    switch(_type) {
        case Type::Object:
            return objFromValue()->setValue(v);
        case Type::Ref:
            return objFromValue()->setProperty(_id, v);
        default:
            return false;
    }
}

