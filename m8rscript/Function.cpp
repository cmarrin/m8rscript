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

#include "Function.h"

#include "ExecutionUnit.h"

using namespace m8r;

CallReturnValue Function::call(ExecutionUnit* eu, uint32_t nparams)
{
    eu->startFunction(objectId(), nparams);
    return CallReturnValue(CallReturnValue::Type::FunctionStart);
}

int32_t Function::addLocal(const Atom& atom)
{
    for (auto name : _locals) {
        if (name == atom) {
            return -1;
        }
    }
    _locals.push_back(atom);
    return static_cast<int32_t>(_locals.size()) - 1;
}

int32_t Function::localIndex(const Atom& name) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(_locals.size()); ++i) {
        if (_locals[i] == name) {
            return i;
        }
    }
    return -1;
}

ConstantId Function::addConstant(const Value& v)
{
    assert(_constants.size() < std::numeric_limits<uint8_t>::max());
    
    for (ConstantId::value_type id = 0; id < _constants.size(); ++id) {
        if (_constants[id] == v) {
            return ConstantId(id);
        }
    }
    
    ConstantId r(static_cast<ConstantId::value_type>(_constants.size()));
    _constants.push_back(v);
    return r;
}

bool Function::serialize(Stream* stream, Error& error, Program* program) const
{
    if (!serializeWrite(stream, error, ObjectDataType::ObjectStart)) {
        return false;
    }

    const char* name = typeName();
    if (!serializeBuffer(stream, error, ObjectDataType::ObjectName, reinterpret_cast<const uint8_t*>(name), strlen(name))) {
        return false;
    }
    
    if (!serializeContents(stream, error, program)) {
        return false;
    }

    return serializeWrite(stream, error, ObjectDataType::ObjectEnd);
}

bool Function::deserialize(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& stringTable)
{
    ObjectDataType type;
    if (!deserializeRead(stream, error, type) || type != ObjectDataType::ObjectStart) {
        return false;
    }

    uint16_t size;
    if (!deserializeBufferSize(stream, error, ObjectDataType::ObjectName, size)) {
        return false;
    }
    
    uint8_t* name = static_cast<uint8_t*>(malloc(size + 1));
    if (!deserializeBuffer(stream, error, name, size)) {
        return false;
    }
    name[size] = '\0';

    if (strcmp(reinterpret_cast<const char*>(name), typeName()) != 0) {
        free(name);
        return false;
    }
    free(name);
    
    if (!deserializeContents(stream, error, program, atomTable, stringTable)) {
        return false;
    }

    if (!deserializeRead(stream, error, type) || type != ObjectDataType::ObjectEnd) {
        return false;
    }
    return true;
}

bool Function::serializeContents(Stream* stream, Error& error, Program* program) const
{
    if (!MaterObject::serialize(stream, error, program)) {
        return false;
    }
    if (!serializeBuffer(stream, error, ObjectDataType::Locals, 
                         _locals.size() ? reinterpret_cast<const uint8_t*>(&(_locals[0])) : nullptr, 
                         _locals.size() * sizeof(uint16_t))) {
        return false;
    }

    size_t size = _code.size();
    return serializeBuffer(stream, error, ObjectDataType::Code, 
                            _code.size() ? reinterpret_cast<const uint8_t*>(_code.data()) : nullptr, size);
}

bool Function::deserializeContents(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& stringTable)
{
    if (!MaterObject::deserialize(stream, error, program, atomTable, stringTable)) {
        return false;
    }

    _locals.clear();

    uint16_t size;
    if (!deserializeBufferSize(stream, error, ObjectDataType::Locals, size)) {
        return false;
    }
    
    assert((size & 1) == 0);
    _locals.resize(size / 2);
    if (!deserializeBuffer(stream, error, 
                           _locals.size() ? reinterpret_cast<uint8_t*>(&(_locals[0])) : nullptr, size)) {
        return false;
    }
    
    _code.clear();

    if (!deserializeBufferSize(stream, error, ObjectDataType::Code, size)) {
        return false;
    }
    
    _code.resize(size);
    if (!deserializeBuffer(stream, error, reinterpret_cast<uint8_t*>(_code.data()), size)) {
        return false;
    }
    
    // Walk through the code, finding all atoms and strings. Find their values
    // in their respective passed tables and insert the new values into the
    // passed Program.
    for (uint32_t i = 0; i < _code.size(); ) {
        Op op = static_cast<Op>(_code[i++].op());
        if (op == Op::END) {
            break;
        }
        
// FIXME: Now we just need to walk through the Constants array

// FIXME: implement for RegisterBasedVM
//
//        if (op < Op::PUSHI) {
//            uint32_t count = ExecutionUnit::sizeFromOp(op);
//            
//            op = ExecutionUnit::maskOp(op, 0x03);
//            // FIXME: Need to handle PUSHO and Object table
//            if (op == Op::PUSHID) {
//                if (count != 2) {
//                    return false;
//                }
//                uint16_t id = ExecutionUnit::uintFromCode(_code.data(), i, count);
//                String idString = atomTable.stringFromAtom(Atom(id));
//                Atom atom = program->atomizeString(idString.c_str());
//                _code[i++] = ExecutionUnit::byteFromInt(atom.raw(), 1);
//                _code[i++] = ExecutionUnit::byteFromInt(atom.raw(), 0);
//            } else if (op == Op::PUSHSX) {
//                if (count != 4) {
//                    return false;
//                }
//                uint32_t index = ExecutionUnit::uintFromCode(_code.data(), i, count);
//                if (index >= stringTable.size()) {
//                    return false;
//                }
//                const char* str = &(stringTable[index]);
//                StringLiteral stringId = program->addStringLiteral(str);
//                _code[i++] = ExecutionUnit::byteFromInt(stringId.raw(), 3);
//                _code[i++] = ExecutionUnit::byteFromInt(stringId.raw(), 2);
//                _code[i++] = ExecutionUnit::byteFromInt(stringId.raw(), 1);
//                _code[i++] = ExecutionUnit::byteFromInt(stringId.raw(), 0);
//            } else {
//                i += count;
//            }
//        }
    }
    return true;
}
