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

#include "Object.h"

#include "ExecutionUnit.h"
#include "Function.h"
#include "MStream.h"
#include "Program.h"

using namespace m8r;

bool Object::serializeBuffer(Stream* stream, Error& error, ObjectDataType type, const uint8_t* buffer, size_t size) const
{
    if (!serializeWrite(stream, error, type)) {
        return error.setError(Error::Code::Write);
    }
    assert(size < 65536);
    uint16_t ssize = static_cast<uint16_t>(size);
    if (!serializeWrite(stream, error, ssize)) {
        return error.setError(Error::Code::Write);
    }
    for (uint16_t i = 0; i < ssize; ++i) {
        if (!serializeWrite(stream, error, buffer[i])) {
            return error.setError(Error::Code::Write);
        }
    }
    return true;
}

bool Object::deserializeBufferSize(Stream* stream, Error& error, ObjectDataType expectedType, uint16_t& size) const
{
    ObjectDataType type;
    if (!deserializeRead(stream, error, type) || type != expectedType) {
        return error.setError(Error::Code::Read);
    }
    if (!deserializeRead(stream, error, size)) {
        return error.setError(Error::Code::Read);
    }
    return true;
}

bool Object::deserializeBuffer(Stream* stream, Error& error, uint8_t* buffer, uint16_t size) const
{
    for (uint16_t i = 0; i < size; ++i) {
        if (!deserializeRead(stream, error, buffer[i])) {
            return error.setError(Error::Code::Read);
        }
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, Error& error, ObjectDataType value) const
{
    uint8_t c = static_cast<uint8_t>(value);
    if (stream->write(c) != c) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, Error& error, uint8_t value) const
{
    if (stream->write(value) != value) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, Error& error, uint16_t value) const
{
    uint8_t c = static_cast<uint8_t>(value >> 8);
    if (stream->write(c) != c) {
        return error.setError(Error::Code::Write);
    }
    c = static_cast<uint8_t>(value);
    if (stream->write(c) != c) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::deserializeRead(Stream* stream, Error& error, ObjectDataType& value) const
{
    int c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value = static_cast<ObjectDataType>(c);
    return true;
}

bool Object::deserializeRead(Stream* stream, Error& error, uint8_t& value) const
{
    int c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value = static_cast<uint8_t>(c);
    return true;
}

bool Object::deserializeRead(Stream* stream, Error& error, uint16_t& value) const
{
    int c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value = (static_cast<uint16_t>(c) & 0xff) << 8;
    c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value |= static_cast<uint16_t>(c) & 0xff;
    return true;
}

bool Object::serializeObject(Stream* stream, Error& error, Program* program) const
{
    if (!serializeWrite(stream, error, ObjectDataType::Type)) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, static_cast<uint8_t>('m'))) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, static_cast<uint8_t>('8'))) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, static_cast<uint8_t>('r'))) {
        return error.setError(Error::Code::Write);
    }
    
    if (!serializeWrite(stream, error, ObjectDataType::Version)) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, MajorVersion)) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, MinorVersion)) {
        return error.setError(Error::Code::Write);
    }

    if (!serialize(stream, error, program)) {
        return false;
    }

    if (!serializeWrite(stream, error, ObjectDataType::End)) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::deserializeObject(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& stringTable)
{
    ObjectDataType type;
    if (!deserializeRead(stream, error, type)) {
        return false;
    }
    if (type != ObjectDataType::Type) {
        return error.setError(Error::Code::SerialHeader);
    }
    
    uint8_t c;
    if (!deserializeRead(stream, error, c)) {
        return false;
    }
    if (c != 'm') {
        return error.setError(Error::Code::SerialHeader);
    }
    if (!deserializeRead(stream, error, c)) {
        return false;
    }
    if (c != '8') {
        return error.setError(Error::Code::SerialHeader);
    }
    if (!deserializeRead(stream, error, c)) {
        return false;
    }
    if (c != 'r') {
        return error.setError(Error::Code::SerialHeader);
    }
    
    if (!deserializeRead(stream, error, type)) {
        return false;
    }
    if (type != ObjectDataType::Version) {
        return error.setError(Error::Code::SerialType);
    }
    
    uint8_t majorVersion, minorVersion;
    if (!deserializeRead(stream, error, majorVersion)) {
        return false;
    }
    if (majorVersion != MajorVersion) {
        return error.setError(Error::Code::SerialVersion);
    }
    if (!deserializeRead(stream, error, minorVersion)) {
        return false;
    }
    if (minorVersion != MinorVersion) {
        return error.setError(Error::Code::SerialVersion);
    }

    if (!deserialize(stream, error, program, atomTable, stringTable)) {
        return false;
    }
    
    if (!deserializeRead(stream, error, type)) {
        return false;
    }
    if (type != ObjectDataType::End) {
        return error.setError(Error::Code::SerialType);
    }
    return true;
}

String MaterObject::toString(ExecutionUnit* eu) const
{
    // FIXME: Pretty print
    String s = "{ ";
    bool first = true;
    for (auto prop : _properties) {
        if (!first) {
            s += ", ";
        } else {
            first = false;
        }
        s += eu->program()->stringFromAtom(prop.key);
        s += " : ";
        s += prop.value.toStringValue(eu);
    }
    s += " }";
    return s;
}

bool MaterObject::serialize(Stream* stream, Error& error, Program* program) const
{
    // Write the Function properties
    if (!serializeWrite(stream, error, ObjectDataType::PropertyCount)) {
        return false;
    }
    if (!serializeWrite(stream, error, static_cast<uint16_t>(2))) {
        return false;
    }
    if (!serializeWrite(stream, error, static_cast<uint16_t>(_properties.size()))) {
        return false;
    }
    for (auto entry : _properties) {
        Object* obj = program->obj(entry.value);
        // Only store functions
        if (!obj || !obj->code()) {
            continue;
        }
        if (!serializeWrite(stream, error, ObjectDataType::PropertyId)) {
            return false;
        }
        if (!serializeWrite(stream, error, static_cast<uint16_t>(2))) {
            return false;
        }
        if (!serializeWrite(stream, error, entry.key.raw())) {
            return false;
        }
        if (!obj->serialize(stream, error, program)) {
            return false;
        }
    }
    
    return true;
}

bool MaterObject::deserialize(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& stringTable)
{
    // Read the Function Properties
    ObjectDataType type;
    if (!deserializeRead(stream, error, type) || type != ObjectDataType::PropertyCount) {
        return false;
    }
    uint16_t count;
    if (!deserializeRead(stream, error, count) || count != 2) {
        return false;
    }
    if (!deserializeRead(stream, error, count)) {
        return false;
    }
    while (count-- > 0) {
        if (!deserializeRead(stream, error, type) || type != ObjectDataType::PropertyId) {
            return false;
        }
        uint16_t id;
        if (!deserializeRead(stream, error, id) || id != 2) {
            return false;
        }
        if (!deserializeRead(stream, error, id)) {
            return false;
        }
        Function* function = new Function();
        if (!function->deserialize(stream, error, program, atomTable, stringTable)) {
            delete function;
            return false;
        }

        // Convert id into space of the current Program
        String idString = atomTable.stringFromAtom(Atom(id));
        Atom atom = program->atomizeString(idString.c_str());
        
        ObjectId functionId = program->addObject(function);
        function->setObjectId(functionId);
        _properties.push_back({ atom.raw(), functionId });
    }
    
    return true;
}
