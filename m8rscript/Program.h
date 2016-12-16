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

    void setStack(Object* stack) { _objectStore.set(ObjectId(StackId), stack); }
    
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
    
    ObjectId addObject(Object* obj, bool collectable) { obj->setCollectable(collectable); return _objectStore.add(obj); }
    StringId createString() { return _stringStore.add(new String()); }
    
    bool isValid(const ObjectId& id) const { return _objectStore.isValid(id); }
    bool isValid(const StringId& id) const { return _stringStore.isValid(id); }
    
    Object* obj(const Value& value) const
    {
        ObjectId id = value.asObjectIdValue();
        return id ? obj(id) : nullptr;
    }
    Object* obj(const ObjectId& id) const { return _objectStore.ptr(id); }
    
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
        String* s = _stringStore.ptr(id);
        return s ? *s : *_stringStore.ptr(StringId(0));
    }
    
    String& str(const StringId& id)
    {
        // _strings[0] contains an error entry for when invalid ids are passed
        String* s = _stringStore.ptr(id);
        return s ? *s : *_stringStore.ptr(StringId(0));
    }
    
    void gc(ExecutionUnit*);
    void gcMarkValue(ExecutionUnit*, const Value& value);
    
protected:
    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

private:
    // The Id of the stack. This will be filled in with the stack of the current ExecutionUnit with the
    // setStackId() call.
    static constexpr ObjectId::value_type StackId = 1; // First value after the Program itself.
    
    AtomTable _atomTable;
    
    template<typename IdType, typename ValueType> class IdStore {
    public:
        IdType add(ValueType*);
        void remove(IdType);
        bool isValid(const IdType& id) const { return id.raw() < _values.size(); }
        ValueType* ptr(const IdType& id) const { return isValid(id) ? _values[id.raw()] : nullptr; }
        void set(IdType id, ValueType* value) { assert(isValid(id)); _values[id.raw()] = value; }
        
        void gcClear() { _valueMarked.clear(); _valueMarked.resize(_values.size()); }
        void gcMark(IdType id) { _valueMarked[id.raw()] = true; }
        
        bool isGCMarked(IdType id) { return _valueMarked[id.raw()]; }

        void gcSweep()
        {
            for (uint16_t i = 1; i < _values.size(); ++i) {
                if (_values[i] && !_valueMarked[i]) {
                    remove(IdType(i));
                }
            }
        }

    private:
        std::vector<ValueType*> _values;
        std::vector<bool> _valueMarked;
        typename IdType::value_type _freeValueIdCount = 0;
        IdType _freeValueId;
    };
    
    std::vector<char> _stringLiteralTable;
    IdStore<StringId, String> _stringStore;
    IdStore<ObjectId, Object> _objectStore;

    Global _global;
};

template<typename IdType, typename ValueType>
IdType Program::IdStore<IdType, ValueType>::add(ValueType* value)
{
    if (_freeValueId) {
        assert(_freeValueIdCount);
        IdType id = _freeValueId;
        _freeValueId = IdType();
        _freeValueIdCount--;
        return id;
    } else if (_freeValueIdCount) {
        for (uint32_t i = 0; i < _values.size(); ++i) {
            if (!_values[i]) {
                _values[i] = value;
                _freeValueIdCount--;
                return IdType(i);
            }
        }
        assert(false);
        return IdType();
    }
    
    IdType id(_values.size());
    _values.push_back(value);
    _valueMarked.resize(_values.size());
    _valueMarked[id.raw()] = true;
    return id;
}

template<typename IdType, typename ValueType>
void Program::IdStore<IdType, ValueType>::remove(IdType id)
{
    assert(id && id.raw() < _values.size() && _values[id.raw()]);
    
    delete _values[id.raw()];
    _values[id.raw()] = nullptr;
    _freeValueIdCount++;
    _freeValueId = id;
}

template<>
inline void Program::IdStore<ObjectId, Object>::gcSweep()
{
    for (uint16_t i = 1; i < _values.size(); ++i) {
        if (_values[i] && !_valueMarked[i] && _values[i]->collectable()) {
            remove(ObjectId(i));
        }
    }
}


}
