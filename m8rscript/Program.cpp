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

using namespace m8r;

AtomTable Program::_atomTable;

Program::Program(SystemInterface* system) : _global(system)
{
}

Program::~Program()
{
}

bool Program::serialize(Stream* stream, Error& error) const
{
    // Write the atom table
    const Vector<int8_t>& atomTableString = _atomTable.stringTable();
    if (!serializeBuffer(stream, error, ObjectDataType::AtomTable, reinterpret_cast<const uint8_t*>(&(atomTableString[0])), atomTableString.size())) {
        return false;
    }
        
    // Write the string table
    if (!serializeBuffer(stream, error, ObjectDataType::StringTable, reinterpret_cast<const uint8_t*>(&(_stringTable[0])), _stringTable.size())) {
        return false;
    }

    return Function::serialize(stream, error);
}

bool Program::deserialize(Stream* stream, Error& error)
{
    // Read the atom table
    Vector<int8_t>& atomTableString = _atomTable.stringTable();
    atomTableString.clear();

    uint16_t size;
    if (!deserializeBufferSize(stream, error, ObjectDataType::AtomTable, size)) {
        return false;
    }
    
    atomTableString.resize(size);
    if (!deserializeBuffer(stream, error, reinterpret_cast<uint8_t*>(&(atomTableString[0])), size)) {
        return false;
    }
    
    // Read the string table
    _stringTable.clear();

    if (!deserializeBufferSize(stream, error, ObjectDataType::StringTable, size)) {
        return false;
    }
    
    _stringTable.resize(size);
    if (!deserializeBuffer(stream, error, reinterpret_cast<uint8_t*>(&(_stringTable[0])), size)) {
        return false;
    }
    
    return Function::deserialize(stream, error);
}
