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
#include "Function.h"
#include "Global.h"

namespace m8r {

class Program : public Function {
public:
    Program();
    ~Program();

    virtual void gcMark() override
    {
        Function::gcMark();
        _global.nativeObject()->gcMark();
    }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("Program") : Function::toString(eu, false); }

    const Mad<Object> global() const { return _global.nativeObject(); }
    Mad<Object> global() { return _global.nativeObject(); }
        
    String stringFromAtom(const Atom& atom) const { return _atomTable.stringFromAtom(atom); }
    Atom atomizeString(const char* s) const { return _atomTable.atomizeString(s); }
    
    // For debugging
    static void printAtomId(Mad<Program>, int id);

    StringLiteral startStringLiteral() { return StringLiteral(StringLiteral(static_cast<uint32_t>(_stringLiteralTable.size()))); }
    void addToStringLiteral(char c) { _stringLiteralTable.push_back(c); }
    void endStringLiteral() { _stringLiteralTable.push_back('\0'); }
    
    StringLiteral addStringLiteral(const char* s)
    {
        size_t length = strlen(s);
        size_t index = _stringLiteralTable.size();
        _stringLiteralTable.resize(index + length + 1);
        memcpy(&(_stringLiteralTable[index]), s, length + 1);
        return StringLiteral(static_cast<uint32_t>(index));
    }
    const char* stringFromStringLiteral(const StringLiteral& id) const { return &(_stringLiteralTable[id.raw()]); }
    
    StringLiteral stringLiteralFromString(const char* s)
    {
        const char* table = &_stringLiteralTable[0];
        size_t size = _stringLiteralTable.size();
        
        for (size_t i = 0; i < size; ) {
            // Find the next string
            if (strcmp(s, table + i) == 0) {
                return StringLiteral(static_cast<uint32_t>(i));
            }
            
            i += strlen(table + i) + 1;
        }
        
        // Not found, add it
        return addStringLiteral(s);
    }
    
private:    
    AtomTable _atomTable;
    
    std::vector<char> _stringLiteralTable;

    Global _global;
};

}
