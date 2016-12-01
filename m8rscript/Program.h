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

class Object;
class Function;
class SystemInterface;

class Program : public Function {
public:
    Program(SystemInterface* system);
    ~Program();
    
    virtual const char* typeName() const override { return "Program"; }

    const Object* global() const { return &_global; }
    Object* global() { return &_global; }
    
    SystemInterface* system() const { return _global.system(); }

    void setStack(Object* stack) { _objects[StackId] = stack; }
    
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
    
    ObjectId addObject(Object* obj)
    {
        ObjectId id(_objects.size());
        _objects.push_back(obj);
        return id;
    }
    
    StringId createString() {
        StringId id(_strings.size());
        _strings.push_back(String());
        return id;
    }
    
    bool isValid(const ObjectId& id) const { return id.raw() < _objects.size(); }
    bool isValid(const StringId& id) const { return id.raw() < _strings.size(); }
    
    Object* obj(const Value& value) const
    {
        ObjectId id = value.asObjectIdValue();
        return id ? obj(id) : nullptr;
    }
    
    Object* obj(const ObjectId& id) const
    {
        return (id.raw() < _objects.size()) ? _objects[id.raw()] : nullptr;
    }
    
    Function* func(const Value& value) const { return value.isObjectId() ? func(value.asObjectIdValue()) : nullptr; }
    Function* func(const ObjectId& id) const
    {
        Object* object = obj(id);
        return object ? (object->isFunction() ? static_cast<Function*>(object) : nullptr) : nullptr;
    }
    
    static ObjectId stackId() { return ObjectId(StackId); }
    
    const String& str(const Value& value) const
    {
        return str(value.asStringIdValue());
    }
    
    String& str(const Value& value)
    {
        return str(value.asStringIdValue());
    }
    
    const String& str(const StringId& id) const
    {
        // _strings[0] contains an error entry for when invalid ids are passed
        return _strings[(id.raw() < _strings.size()) ? id.raw() : 0];
    }
    
    String& str(const StringId& id)
    {
        // _strings[0] contains an error entry for when invalid ids are passed
        return _strings[(id.raw() < _strings.size()) ? id.raw() : 0];
    }
    
protected:
    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

private:
    // The Id of the stack. This will be filled in with the stack of the current ExecutionUnit with the
    // setStackId() call.
    static constexpr ObjectId::value_type StackId = 1; // First value after the Program itself.
    
    AtomTable _atomTable;
    
    std::vector<char> _stringLiteralTable;
    std::vector<String> _strings;
    std::vector<Object*> _objects;
    Global _global;
};
    
}
