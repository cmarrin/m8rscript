/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Function.h"

namespace m8r {

class UpValue {
public:
    UpValue()
        : _closed(false)
        , _marked(true)
        , _destroyed(false)
    {
    }

    ~UpValue()
    {
        assert(!_destroyed);
        _destroyed = true;
    }
    
    bool closed() const { return _closed; }
    void setClosed(bool v) { _closed = v; }
    bool marked() const { return _marked; }
    void setMarked(bool v) { _marked = v; }
    
    void setStackIndex(uint32_t index) { _value = Value(static_cast<int32_t>(index)); }
    
    Value& value() { return _value; }
    const Value& value() const { return _value; }
    
    bool closeIfNeeded(ExecutionUnit*, uint32_t frame);
    
    uint32_t stackIndex() const { return static_cast<uint32_t>(_value.asIntValue()); }
    
private:
    Value _value;
    bool _closed : 1;
    bool _marked : 1;
    bool _destroyed : 1;
};

class Closure : public Object {
public:
    Closure() { }
    virtual ~Closure();
    
    void init(ExecutionUnit* eu, const Value& function, const Value& thisValue);
    
    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Closure") : Object::toString(eu, false); }

    virtual void gcMark() override
    {
        if (isMarked()) {
            return;
        }
        Object::gcMark();
        _func->gcMark();
        _thisValue.gcMark();
        for (auto it : _upValues) {
            it->value().gcMark();
            it->setMarked(true);
        }
    }
    
    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override { return _func->callProperty(eu, prop, nparams); }

    virtual CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor) override;
    
    const InstructionVector* code() const { return _func->code(); }
    virtual uint16_t localCount() const override { return _func->localCount(); }
    virtual const ConstantValueVector*  constants() const override { return _func->constants(); }
    virtual uint16_t formalParamCount() const override { return _func->formalParamCount(); }
    virtual bool loadUpValue(ExecutionUnit* eu, uint32_t index, Value& value) const override;
    virtual bool storeUpValue(ExecutionUnit* eu, uint32_t index, const Value& value) override;
    
    virtual Atom name() const override { return _func->name(); }

private:
    Vector<Mad<UpValue>> _upValues;

    Mad<Function> _func;
    Value _thisValue;
};

}
