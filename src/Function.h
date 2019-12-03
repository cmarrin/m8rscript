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

    virtual bool isFunction() const override { return true; }

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
    void setCode(const Vector<Instruction>& code) { bool retval = _code.assign(code); (void) retval; assert(retval); }

    int32_t addLocal(const Atom& name);
    int32_t localIndex(const Atom& name) const;
    Atom localName(int32_t index) const { return (index < static_cast<int32_t>(_locals.size())) ? _locals[index] : Atom(); }
    virtual uint32_t localSize() const override { return static_cast<uint32_t>(_locals.size()) + _tempRegisterCount; }
    
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;

    void setConstants(const Vector<Value>& constants) { bool retval = _constants.assign(constants); (void) retval; assert(retval); }

    virtual const ConstantValueVector*  constants() const override { return &_constants; }
    
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
    
    InstructionVector _code;
    Vector<Atom> _locals;
    uint32_t _formalParamCount = 0;
    ConstantValueVector _constants;
    uint8_t _tempRegisterCount = 0;
    Atom _name;
};

}
