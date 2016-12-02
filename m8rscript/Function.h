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
    Function()
    {
        // Place a dummy constant at index 0 as an error return value
        _constants.push_back(Value());
    }

    virtual ~Function() { }

    virtual const char* typeName() const override { return "Function"; }
    
    virtual bool isFunction() const override { return true; }

    virtual const Code* code() const override { return &_code; }

    virtual int32_t addLocal(const Atom& name) override;
    virtual int32_t localIndex(const Atom& name) const override;
    virtual Atom localName(int32_t index) const override { return (index < static_cast<int32_t>(_locals.size())) ? _locals[index] : Atom(); }
    virtual size_t localSize() const override { return _locals.size(); }
    virtual CallReturnValue call(ExecutionUnit*, uint32_t nparams) override;
    
    ConstantId addConstant(const Value& v)
    {
        assert(_constants.size() < std::numeric_limits<uint8_t>::max());
        ConstantId r(static_cast<ConstantId::value_type>(_constants.size()));
        _constants.push_back(v);
        return r;
    }
    virtual Value constant(ConstantId id) const override { return _constants[(id.raw() < _constants.size()) ? id.raw() : 0]; }
    size_t constantCount() const { return _constants.size(); }

    void addCode(uint32_t c) { _code.push_back(c); }
    void setCodeAtIndex(uint32_t index, uint32_t c) { _code[index] = c; }

    void markParamEnd() { _paramEnd = static_cast<uint32_t>(_locals.size()); }

    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;
    
protected:
    bool serializeContents(Stream*, Error&, Program*) const;
    bool deserializeContents(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&);

private:
    Code _code;
    std::vector<Atom> _locals;
    uint32_t _paramEnd = 0;
    std::vector<Value> _constants;
};
    
}
