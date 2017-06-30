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

class Function : public Object {
public:
    Function(Function* parent);

    virtual ~Function() { }

    virtual bool isFunction() const override { return true; }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Function") : Object::toString(eu, false); }
        
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) override;

    virtual void gcMark(ExecutionUnit* eu) override
    {
        Object::gcMark(eu);
        for (auto it : _constants) {
            it.gcMark(eu);
        }
    }

    virtual const Vector(Instruction)* code() const override { return &_code; }
    Vector(Instruction)* code() { return &_code; }

    int32_t addLocal(const Atom& name);
    int32_t localIndex(const Atom& name) const;
    Atom localName(int32_t index) const { return (index < static_cast<int32_t>(_locals.size())) ? _locals[index] : Atom(); }
    virtual uint32_t localSize() const override { return static_cast<uint32_t>(_locals.size()) + _tempRegisterCount; }
    
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;
    
    ConstantId addConstant(const Value&);

    Value constant(ConstantId id) const { return _constants[id.raw()]; }

    size_t constantCount() const { return _constants.size(); }
    
    virtual const std::vector<Value>*  constants() const override { return &_constants; }
    
    void setName(const Atom s) { _name = s; }
    virtual Atom name() const override { return _name; }
    
    uint32_t addUpValue(uint32_t index, uint16_t frame, Atom name);
    
    uint32_t upValueStackIndex(ExecutionUnit* eu, uint32_t index) const; 
    
    void upValue(uint32_t i, uint32_t& index, uint16_t& frame, Atom& name) const
    {
        index = _upValues[i]._index;
        frame = _upValues[i]._frame;
        name = _upValues[i]._name;
    }
    
    uint32_t upValueCount() const { return static_cast<uint32_t>(_upValues.size()); }

    void markParamEnd() { _formalParamCount = static_cast<uint32_t>(_locals.size()); }
    virtual uint32_t formalParamCount() const override{ return _formalParamCount; }
    
    void setTempRegisterCount(uint8_t n) { _tempRegisterCount = n; }
    uint8_t tempRegisterCount() const { return _tempRegisterCount; }

    virtual bool hasUpValues() const override { return !_upValues.empty(); }

private:
    struct UpValueEntry {
        UpValueEntry(uint32_t index, uint16_t frame, Atom name)
            : _index(index)
            , _frame(frame)
            , _name(name)
        { }
        
        bool operator==(const UpValueEntry& other) const
        {
            return _index == other._index && _frame == other._frame && _name == other._name;
        }
        
        uint32_t _index;
        uint16_t _frame;
        Atom _name;
    };
    
    std::vector<UpValueEntry> _upValues;
    
    Vector(Instruction) _code;
    std::vector<Atom> _locals;
    uint32_t _formalParamCount = 0;
    std::vector<Value> _constants;
    uint8_t _tempRegisterCount = 0;
    Atom _name;
    Function* _parent = nullptr;
};
    
}
