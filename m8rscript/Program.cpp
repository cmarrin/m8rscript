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

#include "Program.h"

#include "ExecutionUnit.h"

using namespace m8r;

Program::Program() : _global(this)
{
}

Program::~Program()
{
}

bool Program::serialize(Stream* stream, Error& error, Program* program) const
{
    // Write the atom table
    const std::vector<int8_t>& atomTableString = _atomTable.stringTable();
    if (!serializeBuffer(stream, error, ObjectDataType::AtomTable, reinterpret_cast<const uint8_t*>(&(atomTableString[0])), atomTableString.size())) {
        return false;
    }
        
    // Write the literal string table
    if (!serializeBuffer(stream, error, ObjectDataType::StringTable, 
                        _stringLiteralTable.size() ? reinterpret_cast<const uint8_t*>(&(_stringLiteralTable[0])) : nullptr, 
                        _stringLiteralTable.size())) {
        return false;
    }
    
    // Write the objects
    if (!serializeWrite(stream, error, ObjectDataType::ObjectCount)) {
        return false;
    }
    if (!serializeWrite(stream, error, static_cast<uint16_t>(2))) {
        return false;
    }
//    if (!serializeWrite(stream, error, static_cast<uint16_t>(_objects.size()))) {
//        return false;
//    }
    
    for (uint16_t i = 0; ; i++) {
        if (!Global::isValid(ObjectId(i))) {
            break;
        }
        Object* object = Global::obj(ObjectId(i));
        
        // Only store functions
        if (!object->isFunction()) {
            continue;
        }
        if (!serializeWrite(stream, error, ObjectDataType::ObjectId)) {
            return false;
        }
        if (!serializeWrite(stream, error, static_cast<uint16_t>(2))) {
            return false;
        }
        if (!serializeWrite(stream, error, i)) {
            return false;
        }
        if (!object->serialize(stream, error, program)) {
            return false;
        }
    }
    
    return Function::serialize(stream, error, program);
}

bool Program::deserialize(Stream* stream, Error& error, Program* program, const AtomTable&, const std::vector<char>&)
{
    assert(!program);
    
    // FIXME: Currently we read the atomTable and stringTable locally and pass them around to allow their ids to be
    // changed in the Program. We need to do the same for Objects. Create a dummy program to store them all in and 
    // pass that around instead. The passed in Program will become the toProgram and the dummy will be the fromProgram
    
    // Read the atom table locally, so we can use it to translate atoms from the code
    // in this stream to the current program
    AtomTable atomTable;
    std::vector<int8_t>& atomTableString = atomTable.stringTable();

    uint16_t size;
    if (!deserializeBufferSize(stream, error, ObjectDataType::AtomTable, size)) {
        return false;
    }
    
    atomTableString.resize(size);
    if (!deserializeBuffer(stream, error, reinterpret_cast<uint8_t*>(&(atomTableString[0])), size)) {
        return false;
    }
    
    // Read the string table locally, so we can use it to add strings to the existing program
    std::vector<char> stringTable;

    if (!deserializeBufferSize(stream, error, ObjectDataType::StringTable, size)) {
        return false;
    }
    
    stringTable.resize(size);
    if (!deserializeBuffer(stream, error, reinterpret_cast<uint8_t*>(&(stringTable[0])), size)) {
        return false;
    }

    // Read the objects
    ObjectDataType type;
    if (!deserializeRead(stream, error, type) || type != ObjectDataType::ObjectCount) {
        return false;
    }
    if (!deserializeRead(stream, error, size) || size != 2) {
        return false;
    }
    uint16_t count;
    if (!deserializeRead(stream, error, count)) {
        return false;
    }
//    _objects.resize(count);
//    while (count-- > 0) {
//        if (!deserializeRead(stream, error, type) || type != ObjectDataType::ObjectId) {
//            return false;
//        }
//        uint16_t id;
//        if (!deserializeRead(stream, error, id) || id != 2) {
//            return false;
//        }
//        if (!deserializeRead(stream, error, id)) {
//            return false;
//        }
//        
//        // FIXME: We need to convert to the id space of the destination Program
//        Function* function = new Function();
//        function->setObjectId(id);
//        
//        if (!function->deserialize(stream, error, this, atomTable, stringTable)) {
//            delete function;
//            return false;
//        }
//        
//        _objects[id] = function;
//    }
    
    return Function::deserialize(stream, error, this, atomTable, stringTable);
}
