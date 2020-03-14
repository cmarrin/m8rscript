/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Function") : Object::toString(eu, false); }
        
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) override;

    virtual void gcMark() override
    {
        if (isMarked()) {
            return;
        }
        Object::gcMark();
        for (auto it : _constants) {
            it.gcMark();
        }
    }

    virtual const InstructionVector* code() const override { return &_code; }
    void setCode(const Vector<uint8_t>& code) { bool retval = _code.assign(code); (void) retval; assert(retval); }

    void setLocalCount(uint16_t size) { _localSize = size; }
    virtual uint16_t localCount() const override { return _localSize; }
    
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams) override;

    void setConstants(const Vector<Value>& constants) { bool retval = _constants.assign(constants); (void) retval; assert(retval); }

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

    void setName(const Atom s) { _name = s; }
    virtual Atom name() const override { return _name; }
    
    uint32_t addUpValue(uint32_t index, uint16_t frame, Atom name);
        
    virtual bool upValue(uint32_t i, uint32_t& index, uint16_t& frame, Atom& name) const override
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

private:
    struct UpValueEntry {
        UpValueEntry() { }
        UpValueEntry(uint32_t index, uint16_t frame, Atom name)
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
        Atom _name;
    };
    
    Vector<UpValueEntry> _upValues;
    uint16_t _formalParamCount = 0;
    InstructionVector _code;
    uint16_t _localSize = 0;
    FixedVector<Value> _constants;
    Atom _name;
};

}
