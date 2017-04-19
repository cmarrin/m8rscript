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

#include "Function.h"
#include "Object.h"
#include "Containers.h"
#include "Atom.h"

namespace m8r {

class Closure : public Object {
public:
    Closure(ExecutionUnit* eu, const Value& function, const Value& thisValue);
    virtual ~Closure() { }
    
    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Closure") : Object::toString(eu, false); }

    virtual void gcMark(ExecutionUnit* eu) override
    {
        _func->gcMark(eu);
        for (auto it : _upValues) {
            it.value.gcMark(eu);
        }
    }
    
    void close(ExecutionUnit*);

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override { return _func->callProperty(eu, prop, nparams); }

    virtual CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor) override;
    
    virtual bool serialize(Stream* stream, Error& error, Program* program) const override { return _func->serialize(stream, error, program); }
    virtual bool deserialize(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& array) override
    {
        return _func->deserialize(stream, error, program, atomTable, array);
    }

    virtual const Code* code() const override { return _func->code(); }
    virtual uint32_t localSize() const override { return _func->localSize(); }
    virtual const std::vector<Value>*  constants() const override { return _func->constants(); }
    virtual uint32_t formalParamCount() const override { return _func->formalParamCount(); }
    virtual bool loadUpValue(ExecutionUnit* eu, uint32_t index, Value& value) const override;
    virtual bool storeUpValue(ExecutionUnit* eu, uint32_t index, const Value& value) override;
    
    virtual Atom name() const override { return _func->name(); }

private:
    struct UpValue {
        UpValue(uint32_t index) : stackIndex(index) { }
        bool closed = false;
        uint32_t stackIndex = 0;
        Value value;
    };
    
    std::vector<UpValue> _upValues;

    Function* _func = nullptr;
    Value _thisValue;
};

}
