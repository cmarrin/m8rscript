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

#include "Stream.h"

using namespace m8r;

bool Object::serializeBuffer(Stream* stream, ObjectDataType type, const uint8_t* buffer, size_t size) const
{
    if (!serializeWrite(stream, type)) {
        return false;
    }
    assert(size < 65536);
    uint16_t ssize = static_cast<uint16_t>(size);
    if (!serializeWrite(stream, ssize)) {
        return false;
    }
    for (uint16_t i = 0; i < ssize; ++i) {
        if (!serializeWrite(stream, buffer[i])) {
            return false;
        }
    }
    return true;
}

bool Object::deserializeBufferSize(Stream* stream, ObjectDataType expectedType, uint16_t& size) const
{
    ObjectDataType type;
    if (!deserializeRead(stream, type) || type != expectedType) {
        return false;
    }
    return deserializeRead(stream, size);
}

bool Object::deserializeBuffer(Stream* stream, uint8_t* buffer, uint16_t size) const
{
    for (uint16_t i = 0; i < size; ++i) {
        if (!deserializeRead(stream, buffer[i])) {
            return false;
        }
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, ObjectDataType value) const
{
    uint8_t c = static_cast<uint8_t>(value);
    return stream->write(c) == c;
}

bool Object::serializeWrite(Stream* stream, uint8_t value) const
{
    return stream->write(value) == value;
}

bool Object::serializeWrite(Stream* stream, uint16_t value) const
{
    uint8_t c = static_cast<uint8_t>(value >> 8);
    if (stream->write(c) != c) {
        return false;
    }
    c = static_cast<uint8_t>(value);
    return stream->write(c) == c;
}

bool Object::deserializeRead(Stream* stream, ObjectDataType& value) const
{
    int c = stream->read();
    if (c < 0) {
        return false;
    }
    value = static_cast<ObjectDataType>(c);
    return true;
}

bool Object::deserializeRead(Stream* stream, uint8_t& value) const
{
    int c = stream->read();
    if (c < 0) {
        return false;
    }
    value = static_cast<uint8_t>(c);
    return true;
}

bool Object::deserializeRead(Stream* stream, uint16_t& value) const
{
    int c = stream->read();
    if (c < 0) {
        return false;
    }
    value = (static_cast<uint16_t>(c) & 0xff) << 8;
    c = stream->read();
    if (c < 0) {
        return false;
    }
    value |= static_cast<uint16_t>(c) & 0xff;
    return true;
}

bool Object::serializeObject(Stream* stream) const
{
    if (!serializeWrite(stream, ObjectDataType::Version)) {
        return false;
    }
    if (!serializeWrite(stream, MajorVersion)) {
        return false;
    }
    if (!serializeWrite(stream, MinorVersion)) {
        return false;
    }
    const char* name = typeName();
    if (!serializeBuffer(stream, ObjectDataType::Name, reinterpret_cast<const uint8_t*>(name), strlen(name))) {
        return false;
    }
    
    if (!serialize(stream)) {
        return false;
    }
    if (!serializeWrite(stream, ObjectDataType::End)) {
        return false;
    }
    return true;
}

bool Object::deserializeObject(Stream* stream)
{
    ObjectDataType type;
    if (!deserializeRead(stream, type) || type != ObjectDataType::Version) {
        return false;
    }
    uint8_t majorVersion, minorVersion;
    if (!deserializeRead(stream, majorVersion) || majorVersion != MajorVersion) {
        return false;
    }
    if (!deserializeRead(stream, minorVersion) || minorVersion != MinorVersion) {
        return false;
    }
    
    uint16_t size;
    if (!deserializeBufferSize(stream, ObjectDataType::Name, size)) {
        return false;
    }
    
    uint8_t* typeName = static_cast<uint8_t*>(malloc(size));
    if (!deserializeBuffer(stream, typeName, size)) {
        return false;
    }

    // FIXME: Do something with the typeName;
    free(typeName);
    
    if (!deserialize(stream)) {
        return false;
    }
    
    if (!deserializeRead(stream, type) || type != ObjectDataType::End) {
        return false;
    }
    return true;
}
