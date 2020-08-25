/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "MachineCode.h"
#include "Object.h"

namespace m8rscript {

class Function : public MaterObject {
public:
    Function();

    virtual ~Function() { }

    virtual m8r::String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? m8r::String("Function") : Object::toString(eu, false); }
        
    virtual m8r::CallReturnValue callProperty(ExecutionUnit*, m8r::Atom prop, uint32_t nparams) override;

    virtual void gcMark() override
    {
        if (isMarked()) {
            return;
        }
        MaterObject::gcMark();
        for (auto it : _constants) {
            it.gcMark();
        }
    }

    virtual const InstructionVector* code() const override { return &_code; }
    void setCode(const m8r::Vector<uint8_t>& code) { _code = code; }

    void setLocalCount(uint16_t size) { _localSize = size; }
    virtual uint16_t localCount() const override { return _localSize; }
    
    virtual m8r::CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams) override;

    void setConstants(const m8r::Vector<Value>& constants) { _constants = constants; }

    void enumerateConstants(std::function<void(const Value&, const ConstantId&)> func)
    {
        for (uint8_t i = 0; i < _constants.size(); ++i) {
            func(_constants[i], ConstantId(i));
        }
    }
    
    static bool builtinConstant(uint8_t id, Value& value);
    virtual bool constant(uint8_t reg, Value& value) const override
    {
        if (builtinConstant(reg, value)) {
            return true;
        }
        
        reg -= MaxRegister + 1;
        value = _constants[reg - builtinConstantOffset()];
        return true;
    }

    void setName(const m8r::Atom s) { _name = s; }
    virtual m8r::Atom name() const override { return _name; }
    
    uint32_t addUpValue(uint32_t index, uint16_t frame, m8r::Atom name);
        
    virtual bool upValue(uint32_t i, uint32_t& index, uint16_t& frame, m8r::Atom& name) const override
    {
        index = _upValues[i]._index;
        frame = _upValues[i]._frame;
        name = _upValues[i]._name;
        return true;
    }
    
    virtual uint32_t upValueCount() const override { return static_cast<uint32_t>(_upValues.size()); }

    void setFormalParamCount(uint16_t count) { _formalParamCount = count; }
    virtual uint16_t formalParamCount() const override{ return _formalParamCount; }
    virtual bool loadUpValue(ExecutionUnit* eu, uint32_t index, Value& value) const override;

    virtual bool canMakeClosure() const override { return true; }

private:
    struct UpValueEntry {
        UpValueEntry() { }
        UpValueEntry(uint32_t index, uint16_t frame, m8r::Atom name)
            : _index(index)
            , _frame(frame)
            , _name(name)
        { }
        
        bool operator==(const UpValueEntry& other) const
        {
            return _index == other._index && _frame == other._frame && _name == other._name;
        }
        
        uint32_t _index = 0;
        uint16_t _frame = 0;
        m8r::Atom _name;
    };
    
    m8r::Vector<UpValueEntry> _upValues;
    uint16_t _formalParamCount = 0;
    InstructionVector _code;
    uint16_t _localSize = 0;
    m8r::Vector<Value> _constants;
    m8r::Atom _name;
};

}
