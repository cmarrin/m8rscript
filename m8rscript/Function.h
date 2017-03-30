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

class Closure;
class Function;

class Callable {
public:
    enum class Type { None, Function, Closure };
    
    virtual Type type() const { return Type::None; }
    virtual const Code* code() const = 0;
    virtual uint32_t localSize() const = 0;
    virtual Value* constantsPtr() = 0;
    virtual uint32_t formalParamCount() const = 0;
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value&) const = 0;
    virtual bool storeUpValue(ExecutionUnit*, uint32_t index, const Value&) = 0;
    virtual Atom name() const = 0;
    virtual bool hasUpValues() const { return false; }
    
    bool isFunction() const { return type() == Type::Function; }
};

class Function : public Object, public Callable {
public:
    Function(Function* parent);

    virtual ~Function() { }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Function") : Object::toString(eu, false); }
        
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) override;

    virtual void gcMark(ExecutionUnit* eu) override
    {
        Object::gcMark(eu);
        for (auto it : _constants) {
            it.gcMark(eu);
        }
    }

    virtual const Code* code() const override { return &_code; }
    Code* code() { return &_code; }

    int32_t addLocal(const Atom& name);
    int32_t localIndex(const Atom& name) const;
    Atom localName(int32_t index) const { return (index < static_cast<int32_t>(_locals.size())) ? _locals[index] : Atom(); }
    virtual uint32_t localSize() const override { return static_cast<uint32_t>(_locals.size()) + _tempRegisterCount; }
    
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;
    
    ConstantId addConstant(const Value&);

    Value constant(ConstantId id) const { return _constants[id.raw()]; }

    size_t constantCount() const { return _constants.size(); }
    
    virtual Value* constantsPtr() override { return &(_constants.at(0)); }
    
    void setName(const Atom s) { _name = s; }
    virtual Atom name() const override { return _name; }
    
    uint32_t addUpValue(uint32_t index, uint16_t frame, Atom name);
    
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value&) const override;
    virtual bool storeUpValue(ExecutionUnit*, uint32_t index, const Value&) override;
    bool captureUpValue(ExecutionUnit*, uint32_t index, Value&) const;
    
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

    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

    virtual Type type() const override { return Type::Function; }

    virtual bool hasUpValues() const override { return !_upValues.empty(); }

protected:
    bool serializeContents(Stream*, Error&, Program*) const;
    bool deserializeContents(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&);

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
    
    Code _code;
    std::vector<Atom> _locals;
    uint32_t _formalParamCount = 0;
    std::vector<Value> _constants;
    uint8_t _tempRegisterCount = 0;
    Atom _name;
    Function* _parent = nullptr;
};
    
class Closure : public Object, public Callable {
public:
    Closure(ExecutionUnit* eu, Function* func);
    virtual ~Closure() { }
    
    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Closure") : toString(eu, false); }

    virtual void gcMark(ExecutionUnit* eu) override
    {
        _func->gcMark(eu);
        for (auto it : _upvalues) {
            it.gcMark(eu);
        }
    }

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override { return _func->callProperty(eu, prop, nparams); }

    virtual CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor) override;
    
    virtual bool serialize(Stream* stream, Error& error, Program* program) const override { return _func->serialize(stream, error, program); }
    virtual bool deserialize(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& array) override
    {
        return _func->deserialize(stream, error, program, atomTable, array);
    }

    virtual const Code* code() const override { return _func->code(); }
    virtual uint32_t localSize() const override { return _func->localSize(); }
    virtual Value* constantsPtr() override { return _func->constantsPtr(); }
    virtual uint32_t formalParamCount() const override { return _func->formalParamCount(); }
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value& value) const override
    {
        if (index >= _upvalues.size()) {
            return false;
        }
        value = _upvalues[index];
        return true;
    }
    
    virtual bool storeUpValue(ExecutionUnit*, uint32_t index, const Value& value) override
    {
        if (index >= _upvalues.size()) {
            return false;
        }
        _upvalues[index] = value;
        return true;
    }
    
    virtual Atom name() const override { return _func->name(); }
    virtual Type type() const override { return Type::Closure; }

private:    
    Function* _func = nullptr;
    std::vector<Value> _upvalues;
};

}
