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

namespace m8r {

class Object;
class Function;

class ObjectId {
    friend class Program;
    
public:
    uint32_t rawObjectId() const { return _id; }
    int compare(const ObjectId& other) const { return static_cast<int>(_id) - static_cast<int>(other._id); }
    
    static ObjectId objectIdFromRawObjectId(uint32_t rawObjectId) { ObjectId id; id._id = rawObjectId; return id; }
    
private:
    uint32_t _id;
};
    
class Program {
public:
    typedef Map<ObjectId, Object*> ObjectMap;

    Program() { _main = new Function(); }
    
    ~Program();
    
    const Function* main() const { return _main; }
    Function* main() { return _main; }
    
    String stringFromAtom(const Atom& atom) const { return _atomTable.stringFromAtom(atom); }
    String stringFromRawAtom(uint16_t rawAtom) const { return _atomTable.stringFromRawAtom(rawAtom); }
    Atom atomizeString(const char* s) { return _atomTable.atomizeString(s); }
    
    StringId startString() { StringId id; id._id = static_cast<uint32_t>(_stringTable.size()); return id; }
    void addToString(char c) { _stringTable.push_back(c); }
    void endString() { _stringTable.push_back('\0'); }
    
    StringId addString(const char* s)
    {
        size_t length = strlen(s);
        size_t index = _stringTable.size();
        _stringTable.resize(index + length + 1);
        memcpy(&(_stringTable[index]), s, length + 1);
        StringId id;
        id._id = static_cast<uint32_t>(index);
        return id;
    }
    const char* stringFromId(const StringId& id) const { return &(_stringTable[id.rawStringId()]); }
    
    ObjectId addObject(Object* obj)
    {
        ObjectId id;
        id._id = _nextId++;
        _objects.emplace(id, obj);
        return id;
    }
    const ObjectMap& objects() const { return _objects; }
    Object* objectFromObjectId(const ObjectId& id)
    {
        Object* obj;
        return _objects.find(id, obj) ? obj : nullptr;
    }

private:
    AtomTable _atomTable;
    std::vector<char> _stringTable;
    Function* _main;
    ObjectMap _objects;
    uint32_t _nextId = 1;
};
    
}
