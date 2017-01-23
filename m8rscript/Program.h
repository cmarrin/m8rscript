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

#include "Atom.h"
#include "Function.h"
#include "Global.h"

namespace m8r {

class Program : public Function {
public:
    Program();
    ~Program();
    
    virtual const char* typeName() const override { return "Program"; }

    const Object* global() const { return _global.nativeObject(); }
    Object* global() { return _global.nativeObject(); }
        
    String stringFromAtom(const Atom& atom) const { return _atomTable.stringFromAtom(atom); }
    Atom atomizeString(const char* s) const { return _atomTable.atomizeString(s); }
    
    StringLiteral startStringLiteral() { return StringLiteral(StringLiteral(static_cast<uint32_t>(_stringLiteralTable.size()))); }
    void addToStringLiteral(char c) { _stringLiteralTable.push_back(c); }
    void endStringLiteral() { _stringLiteralTable.push_back('\0'); }
    
    StringLiteral addStringLiteral(const char* s)
    {
        size_t length = strlen(s);
        size_t index = _stringLiteralTable.size();
        _stringLiteralTable.resize(index + length + 1);
        memcpy(&(_stringLiteralTable[index]), s, length + 1);
        return StringLiteral(StringLiteral(static_cast<uint32_t>(index)));
    }
    const char* stringFromStringLiteral(const StringLiteral& id) const { return &(_stringLiteralTable[id.raw()]); }
    
protected:
    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

private:    
    AtomTable _atomTable;
    
    std::vector<char> _stringLiteralTable;

    Global _global;
};

}
