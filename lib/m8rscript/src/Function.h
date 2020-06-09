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

enum class BuiltinConstants {
    Undefined = 0,
    Null = 1,
    Int0 = 2,
    Int1 = 3,
    AtomShort = 4,  // Next byte is Atom Id (0-255)
    AtomLong = 5,   // Next 2 bytes are Atom Id (Hi:Lo, 0-65535)
    NumBuiltins = 6
};

static inline uint8_t builtinConstantOffset() { return static_cast<uint8_t>(BuiltinConstants::NumBuiltins); }
static inline bool shortSharedAtomConstant(uint8_t reg) { return reg > MaxRegister && (reg - MaxRegister - 1) == static_cast<uint8_t>(BuiltinConstants::AtomShort); }
static inline bool longSharedAtomConstant(uint8_t reg) { return reg > MaxRegister && (reg - MaxRegister - 1) == static_cast<uint8_t>(BuiltinConstants::AtomLong); }

static inline uint8_t constantSize(uint8_t reg)
{
    if (reg <= MaxRegister) {
        return 0;
    }
    if (shortSharedAtomConstant(reg)) {
        return 1;
    }
    if (longSharedAtomConstant(reg)) {
        return 2;
    }
    return 0;
}

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
        MaterObject::gcMark();
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

    virtual bool canMakeClosure() const override { return true; }

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
