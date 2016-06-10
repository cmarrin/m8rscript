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

#include "ExecutionUnit.h"

#include "Parser.h"
#include <cassert>

using namespace m8r;

uint32_t ExecutionUnit::_nextID = 1;

Label ExecutionUnit::label()
{
    Label label;
    label.label = _currentFunction->codeSize();
    label.uniqueID = _nextID++;
    return label;
}
    
void ExecutionUnit::addString(StringId s)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHSX));
    _currentFunction->addCodeInt(s.rawStringId(), 4);
}

void ExecutionUnit::addCode(uint32_t value)
{
    uint32_t size;
    uint8_t op;
    
    if (value <= 15) {
        _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHI) | value);
        return;
    }
    if (value <= 255) {
        size = 1;
        op = static_cast<uint8_t>(Op::PUSHIX);
    } else if (value <= 65535) {
        size = 2;
        op = static_cast<uint8_t>(Op::PUSHIX) | 0x01;
    } else {
        size = 4;
        op = static_cast<uint8_t>(Op::PUSHIX) | 0x03;
    }
    _currentFunction->addCode(op);
    _currentFunction->addCodeInt(value, size);
}

void ExecutionUnit::addCode(float value)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHF));
    _currentFunction->addCodeInt(*(reinterpret_cast<uint32_t*>(&value)), 4);
}

void ExecutionUnit::addCode(const Atom& atom)
{
    int32_t index = _currentFunction->localValueIndex(atom);
    if (index >= 0) {
        if (index <= 15) {
            _currentFunction->addCode(static_cast<uint8_t>(static_cast<uint8_t>(Op::PUSHL) | index));
        } else {
            assert(index < 65536);
            uint32_t size = (index < 256) ? 1 : 2;
            _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHLX) | (size - 1));
            _currentFunction->addCodeInt(index, size);
        }
    } else {
        _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHID));
        _currentFunction->addCodeInt(atom.rawAtom(), 2);
    }
}

void ExecutionUnit::addCode(Op value)
{
    _currentFunction->addCode(static_cast<uint8_t>(value));
}

void ExecutionUnit::addCode(const ObjectId& value)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHO));
    _currentFunction->addCodeInt(value.rawObjectId(), 4);
}

void ExecutionUnit::addNamedFunction(Function* function, const Atom& name)
{
    function->setName(name);
    assert(name.valid());
    if (!name.valid()) {
        return;
    }
    _currentFunction->setValue(name, Value(function));
}

void ExecutionUnit::addFixupJump(bool cond, Label& label)
{
    label.fixupAddr = static_cast<int32_t>(_currentFunction->codeSize());
    _currentFunction->addCode(static_cast<uint8_t>(cond ? Op::JT : Op::JF) | 1);
    _currentFunction->addCode(0);
    _currentFunction->addCode(0);
}

void ExecutionUnit::addJumpAndFixup(Label& label)
{
    int32_t jumpAddr = label.label - static_cast<int32_t>(_currentFunction->codeSize()) - 1;
    if (jumpAddr >= -127 && jumpAddr <= 127) {
        _currentFunction->addCode(static_cast<uint8_t>(Op::JMP));
        _currentFunction->addCode(static_cast<uint8_t>(jumpAddr));
    } else {
        if (jumpAddr < -32767 || jumpAddr > 32767) {
            printf("JUMP ADDRESS TOO BIG TO LOOP. CODE WILL NOT WORK!\n");
            return;
        }
        _currentFunction->addCode(static_cast<uint8_t>(Op::JMP) | 0x01);
        _currentFunction->addCode(Function::byteFromInt(jumpAddr, 1));
        _currentFunction->addCode(Function::byteFromInt(jumpAddr, 0));
    }
    
    jumpAddr = static_cast<int32_t>(_currentFunction->codeSize()) - label.fixupAddr - 1;
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printf("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n");
        return;
    }
    _currentFunction->setCodeAtIndex(label.fixupAddr + 1, Function::byteFromInt(jumpAddr, 1));
    _currentFunction->setCodeAtIndex(label.fixupAddr + 2, Function::byteFromInt(jumpAddr, 0));
}

void ExecutionUnit::addCodeWithCount(Op value, uint32_t nparams)
{
    assert(nparams < 256);
    assert(value == Op::CALL || value == Op::NEW || value == Op::RET);
    uint8_t code = static_cast<uint8_t>(value);
    
    if (nparams > 15) {
        value = (value == Op::CALL) ? Op::CALLX : ((value == Op::NEW) ? Op::NEWX : Op::RETX);
        _currentFunction->addCode(static_cast<uint8_t>(value));
        _currentFunction->addCodeInt(nparams, 1);
    } else {
        code |= nparams;
        _currentFunction->addCode(code);
    }
}

Value ExecutionUnit::valueFromId(Atom id, Object* obj)
{
    // Start at the current object and walk up the chain
    return Value();
}

void ExecutionUnit::run()
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static void* dispatchTable[] {
        /* 0x00 */    OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x05 */ OP(PUSHID) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x0B */ OP(PUSHF)
        /* 0x0C */ OP(PUSHIX) OP(PUSHIX) OP(UNKNOWN) OP(PUSHIX)
        /* 0x10 */ OP(PUSHSX) OP(PUSHSX) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x14 */ OP(JMP) OP(JMP) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x18 */ OP(JT) OP(JT)  OP(UNKNOWN) OP(UNKNOWN)
        /* 0x1C */ OP(JF) OP(JF) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x20 */ OP(CALLX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x24 */ OP(NEWX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(PUSHO)
        /* 0x2C */ OP(RETX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x30 */ OP(PUSHLX)  OP(PUSHLX)  OP(UNKNOWN)  OP(UNKNOWN)
        
        /* 0x34 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x38 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x3c */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x40 */ OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x48 */      OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x50 */ OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x58 */      OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x60 */ OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)
        /* 0x68 */      OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)
        /* 0x70 */ OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET)
        /* 0x78 */      OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET)
        /* 0x80 */ OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL)
        /* 0x88 */      OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL)

        /* 0x90 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x98 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0xD0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xD8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(obj->codeAtIndex(i++)); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    Object* obj = _currentProgram->main();
    _stack.resize(0);
    int i = 0;
    
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    float floatValue;
    uint32_t size;
    Op op;
    const Value* value;
    
    
    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return;
    L_PUSHID:
        _stack.push_back(valueFromId(Atom::atomFromRawAtom(uintFromCode(obj, i, 2)), obj));
        i += 2;
        DISPATCH;
    L_PUSHF:
        floatValue = floatFromCode(obj, i);
        _stack.push_back(Value(floatValue));
        i += 4;
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            intValue = intFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            intValue = intFromCode(obj, i, size);
            i += size;
        }
        _stack.push_back(Value(intValue));
        DISPATCH;
    L_PUSHSX:
        size = (static_cast<uint8_t>(op) & 0x0f) + 1;
        uintValue = uintFromCode(obj, i, size);
        i += size;
        _stack.push_back(Value(StringId::stringIdFromRawStringId(obj->codeAtIndex(i++))));
        i += uintValue;
        DISPATCH;
    L_PUSHO:
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            intValue = intFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            intValue = intFromCode(obj, i, size);
            i += size;
        }
        intValue = obj->localValueIndex(intValue);
        _stack.push_back(value ? *value : nullptr);
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(obj, i, size);
        op = maskOp(op, 0x01);
        i += size;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        DISPATCH;
    L_RET:
    L_RETX:
        if (op == Op::RETX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        DISPATCH;
    L_OPCODE:
        DISPATCH;
    L_END:
        return;
}

#if SHOW_CODE
static m8r::String toString(float value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%g", value);
    s.set(buf);
    return s;
}

static m8r::String toString(uint32_t value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%u", value);
    s.set(buf);
    return s;
}

uint32_t ExecutionUnit::findAnnotation(uint32_t addr) const
{
    for (auto annotation : annotations) {
        if (annotation.addr == addr) {
            return annotation.uniqueID;
        }
    }
    return 0;
}

void ExecutionUnit::preamble(String& s, uint32_t addr) const
{
    uint32_t uniqueID = findAnnotation(addr);
    if (!uniqueID) {
        indentCode(s);
        return;
    }
    if (_nestingLevel) {
        --_nestingLevel;
        indentCode(s);
        ++_nestingLevel;
    }
    s += "LABEL[";
    s += ::toString(uniqueID);
    s += "]\n";
    indentCode(s);
}
#endif

m8r::String ExecutionUnit::toString() const
{
#if SHOW_CODE
    String outputString;
    
	for (const auto& object : _currentProgram->objects()) {
        if (object.value->hasCode()) {
            outputString += generateCodeString(_nestingLevel, ::toString(object.key.rawObjectId()).c_str(), object.value);
            outputString += "\n";
        }
	}
    
    outputString += generateCodeString(0, "main", _currentProgram->main());
    return outputString;
#else
    return "PROGRAM\n";
#endif
}

#if SHOW_CODE
m8r::String ExecutionUnit::generateCodeString(uint32_t nestingLevel, const char* functionName, Object* obj) const
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static void* dispatchTable[] {
        /* 0x00 */    OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x05 */ OP(PUSHID) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x0B */ OP(PUSHF)
        /* 0x0C */ OP(PUSHIX) OP(PUSHIX) OP(UNKNOWN) OP(PUSHIX)
        /* 0x10 */ OP(PUSHSX) OP(PUSHSX) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x14 */ OP(JMP) OP(JMP) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x18 */ OP(JT) OP(JT)  OP(UNKNOWN) OP(UNKNOWN)
        /* 0x1C */ OP(JF) OP(JF) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x20 */ OP(CALLX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x24 */ OP(NEWX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(PUSHO)
        /* 0x2C */ OP(RETX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x30 */ OP(PUSHLX)  OP(PUSHLX)  OP(UNKNOWN)  OP(UNKNOWN)
        
        /* 0x34 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x38 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x3c */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x40 */ OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x48 */      OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x50 */ OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x58 */      OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x60 */ OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)
        /* 0x68 */      OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)
        /* 0x70 */ OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET)
        /* 0x78 */      OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET)
        /* 0x80 */ OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL)
        /* 0x88 */      OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL)

        /* 0x90 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x98 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0xD0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xD8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(obj->codeAtIndex(i++)); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    m8r::String outputString;

    _nestingLevel = nestingLevel;
	
	m8r::String name;
    if (functionName[0] != '\0') {
        name = "<";
        name += functionName;
        name += ">";
    }
    else if (obj->name() && obj->name()->valid()) {
        name = _parser->stringFromAtom(*obj->name());
    }
    
    indentCode(outputString);
    outputString += "FUNCTION(";
    outputString += name.c_str();
    outputString += ")\n";
    
    _nestingLevel++;

	for (const auto& value : obj->values()) {
        if (value.value.object() && value.value.object()->hasCode()) {
            outputString += generateCodeString(_nestingLevel, "", value.value.object());
            outputString += "\n";
        }
	}
    
    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    int i = 0;
    for ( ; ; ) {
        if (i >= obj->codeSize()) {
            printf("WENT PAST THE END OF CODE\n");
            return outputString;
        }
        if (obj->codeAtIndex(i) == static_cast<uint8_t>(Op::END)) {
            break;
        }
        uint8_t code = obj->codeAtIndex(i++);
        if (code < static_cast<uint8_t>(Op::PUSHI)) {
            int count = (code & 0x03) + 1;
            int nexti = i + count;
            if (code == static_cast<uint8_t>(Op::PUSHSX)) {
                nexti += intFromCode(obj, i, count);
            }
            
            code &= 0xfc;
            Op op = static_cast<Op>(code);
            if (op == Op::JMP || op == Op::JT || op == Op::JF) {
                int32_t addr = intFromCode(obj, i, count);
                Annotation annotation = { static_cast<uint32_t>(i + addr), uniqueID++ };
                annotations.push_back(annotation);
            }
            i = nexti;
        }
    }
    
    i = 0;
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    uint32_t size;
    
    Op op;
    
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_PUSHID:
        preamble(outputString, i - 1);
        strValue = _parser->stringFromRawAtom(uintFromCode(obj, i, 2));
        i += 2;
        outputString += "ID(";
        outputString += strValue.c_str();
        outputString += ")\n";
        DISPATCH;
    L_PUSHF:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        outputString += "FLT(";
        outputString += ::toString(*(reinterpret_cast<float*>(&uintValue)));
        outputString += ")\n";
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(obj, i, size);
            i += size;
        }
        outputString += "INT(";
        outputString += ::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHSX:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        outputString += "STR(\"";
        outputString += _currentProgram->stringFromId(StringId::stringIdFromRawStringId(uintValue));
        outputString += "\")\n";
        DISPATCH;
    L_PUSHO:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        outputString += "OBJ(";
        outputString += ::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHL) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(obj, i, size);
            i += size;
        }
        outputString += "LOCAL(";
        outputString += _parser->stringFromAtom(obj->localName(uintValue));
        outputString += ")\n";
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        preamble(outputString, i - 1);
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(obj, i, size);
        op = maskOp(op, 0x01);
        outputString += (op == Op::JT) ? "JT" : ((op == Op::JF) ? "JF" : "JMP");
        outputString += " LABEL[";
        outputString += ::toString(findAnnotation(i + intValue));
        outputString += "]\n";
        i += size;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        preamble(outputString, i - 1);
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        
        outputString += (maskOp(op, 0x07) == Op::CALL || op == Op::CALLX) ? "CALL" : "NEW";
        outputString += "[";
        outputString += ::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_RET:
    L_RETX:
        preamble(outputString, i - 1);
        if (op == Op::RETX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
    
        outputString += "RET[";
        outputString += ::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_OPCODE:
        preamble(outputString, i - 1);
        outputString += "OP(";
        outputString += stringFromOp(op);
        outputString += ")\n";
        DISPATCH;
    L_END:
        _nestingLevel--;
        preamble(outputString, i - 1);
        outputString += "END\n";
        return outputString;
}

struct CodeMap
{
    Op op;
    const char* s;
};

#undef OP
#define OP(op) { Op::op, #op },

static CodeMap opcodes[] = {
    OP(PUSHID)
    OP(PUSHF)
    OP(PUSHIX)
    OP(PUSHSX)
    
    OP(JMP)
    OP(JT)
    OP(JF)
    
    OP(CALLX)
    OP(NEWX)
    OP(PUSHO)
    OP(RETX)
    OP(PUSHLX)

    OP(PUSHI)
    OP(CALL)
    OP(NEW)
    OP(RET)
    OP(PUSHL)
    
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(DEREF) OP(DEL) OP(END) OP(POP)

    OP(STO) OP(STOMUL) OP(STOADD) OP(STOSUB) OP(STODIV) OP(STOMOD) OP(STOSHL) OP(STOSHR)
    OP(STOSAR) OP(STOAND) OP(STOOR) OP(STOXOR) OP(LOR) OP(LAND) OP(AND) OP(OR)
    OP(XOR) OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE) OP(SHL)
    OP(SHR) OP(SAR) OP(ADD) OP(SUB) OP(MUL) OP(DIV) OP(MOD)
};

const char* ExecutionUnit::stringFromOp(Op op)
{
    for (auto c : opcodes) {
        if (c.op == op) {
            return c.s;
        }
    }
    return "UNKNOWN";
}

void ExecutionUnit::indentCode(String& s) const
{
    for (int i = 0; i < _nestingLevel; ++i) {
        s += "    ";
    }
}

#endif

