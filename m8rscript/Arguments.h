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

#include "Object.h"

namespace m8r {

class Arguments : public Object {
public:
    Arguments(Program*);

    virtual const char* typeName() const override { return "arguments"; }

    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) override;

    virtual const Value property(ExecutionUnit*, const Atom& prop) const override;
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value&, bool add) override;

    virtual Value iterate(ExecutionUnit* eu, int32_t index) const override
    {
        if (index == Object::IteratorCount) {
            return Value(static_cast<int32_t>(argCount(eu)));
        }
        return arg(eu, index);
    }

private:
    uint32_t argCount(ExecutionUnit*) const;
    const Value& arg(ExecutionUnit*, uint32_t index) const;
    Value& arg(ExecutionUnit*, uint32_t index);

    enum class Property : uint8_t { Length };

    Atom _lengthAtom;
};
    
}