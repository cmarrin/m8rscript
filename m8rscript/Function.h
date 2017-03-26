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
#include "Containers.h"
#include "Atom.h"

namespace m8r {

class Function : public MaterObject {
public:
    Function();

    virtual ~Function() { }

    virtual bool isFunction() const override { return true; }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Function") : MaterObject::toString(eu, false); }
        
    virtual void gcMark(ExecutionUnit* eu) override
    {
        MaterObject::gcMark(eu);
        for (auto it : _constants) {
            it.gcMark(eu);
        }
    }

    const Code* code() const { return &_code; }
    Code* code() { return &_code; }

    int32_t addLocal(const Atom& name);
    int32_t localIndex(const Atom& name) const;
    Atom localName(int32_t index) const { return (index < static_cast<int32_t>(_locals.size())) ? _locals[index] : Atom(); }
    uint32_t localSize() const { return static_cast<uint32_t>(_locals.size()) + _tempRegisterCount; }
    
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;
    
    ConstantId addConstant(const Value&);

    Value constant(ConstantId id) const { return _constants[id.raw()]; }

    size_t constantCount() const { return _constants.size(); }
    
    Value* constantsPtr() { return &(_constants.at(0)); }
    
    uint32_t addUpValue(uint32_t index, uint16_t frame);
    
    Value loadUpValue(ExecutionUnit*, uint32_t index) const;
    void storeUpValue(ExecutionUnit*, uint32_t index, const Value&);
    
    Value upValue(uint32_t i) const { return _upValues[i]; }
    size_t upValueCount() const { return _upValues.size(); }

    void markParamEnd() { _formalParamCount = static_cast<uint32_t>(_locals.size()); }
    uint32_t formalParamCount() const { return _formalParamCount; }
    
    void setTempRegisterCount(uint8_t n) { _tempRegisterCount = n; }
    uint8_t tempRegisterCount() const { return _tempRegisterCount; }

    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

protected:
    bool serializeContents(Stream*, Error&, Program*) const;
    bool deserializeContents(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&);

private:
    static CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams);
    NativeFunction _call;

    Code _code;
    std::vector<Atom> _locals;
    uint32_t _formalParamCount = 0;
    std::vector<Value> _constants;
    uint8_t _tempRegisterCount = 0;
    std::vector<Value> _upValues;
};
    
class Closure {
public:
    Closure(const Value& func) : _func(func) { }
    
    void addUpValue(uint32_t index, uint16_t frame) { _upvalues.emplace_back(Value(index, frame)); }
    
    Value upValue(ExecutionUnit*, uint32_t index);
    void setUpValue(ExecutionUnit*, uint32_t index, const Value&);
    void captureUpValue(ExecutionUnit*, uint32_t index);
    
private:
    Value _func;
    std::vector<Value> _upvalues;
};

}
