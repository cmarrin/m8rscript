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

#include "CodePrinter.h"

#include "ExecutionUnit.h"
#include "Float.h"
#include "Program.h"

using namespace m8r;

uint32_t CodePrinter::findAnnotation(uint32_t addr) const
{
    for (auto annotation : annotations) {
        if (annotation.addr == addr) {
            return annotation.uniqueID;
        }
    }
    return 0;
}

void CodePrinter::preamble(String& s, uint32_t addr) const
{
    if (_lineno != _emittedLineNumber) {
        _emittedLineNumber = _lineno;
        if (_nestingLevel) {
            --_nestingLevel;
            indentCode(s);
            ++_nestingLevel;
        }
        s += "LINENO[";
        s += Value::toString(_lineno);
        s += "]\n";
    }
    
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
    s += Value::toString(uniqueID);
    s += "]\n";
    indentCode(s);
}

m8r::String CodePrinter::generateCodeString(const Program* program) const
{    
    return generateCodeString(program, program, "main", 0);
}

inline String regString(uint32_t reg, bool up = false)
{
    if (up) {
        return String("U[") + Value::toString(reg) + "]";
    }
    if (reg <= MaxRegister) {
        return String("R[") + Value::toString(reg) + "]";
    }
    return String("K[") + Value::toString(reg - MaxRegister - 1) + "]";
}

void CodePrinter::generateXXX(m8r::String& str, uint32_t addr, Op op) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + "\n";
}

void CodePrinter::generateRXX(m8r::String& str, uint32_t addr, Op op, uint32_t d) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(d) + "\n";
}

void CodePrinter::generateRRX(m8r::String& str, uint32_t addr, Op op, uint32_t d, uint32_t s) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(d) + ", " + regString(s) + "\n";
}

void CodePrinter::generateRUX(m8r::String& str, uint32_t addr, Op op, uint32_t d, uint32_t s) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(d) + ", " + regString(s, true) + "\n";
}

void CodePrinter::generateURX(m8r::String& str, uint32_t addr, Op op, uint32_t d, uint32_t s) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(d, true) + ", " + regString(s) + "\n";
}

void CodePrinter::generateRRR(m8r::String& str, uint32_t addr, Op op, uint32_t d, uint32_t s1, uint32_t s2) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(d) + ", " + regString(s1) + ", " + regString(s2) + "\n";
}

void CodePrinter::generateXN(m8r::String& str, uint32_t addr, Op op, int32_t n) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + Value::toString(n) + "\n";
}

void CodePrinter::generateRN(m8r::String& str, uint32_t addr, Op op, uint32_t d, int32_t n) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(d) + ", " + Value::toString(n) + "\n";
}

void CodePrinter::generateCall(m8r::String& str, uint32_t addr, Op op, uint32_t rcall, uint32_t rthis, int32_t nparams) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(rcall) + ", " + regString(rthis) + ", " + Value::toString(nparams) + "\n";
}

m8r::String CodePrinter::generateCodeString(const Program* program, const Object* func, const char* functionName, uint32_t nestingLevel) const
{
    #undef OP
    #define OP(op) &&L_ ## op,
    static const void* dispatchTable[] {
        /* 0x00 */ OP(MOVE) OP(LOADREFK) OP(STOREFK) OP(LOADLITA)
        /* 0x04 */ OP(LOADLITO) OP(LOADPROP) OP(LOADELT) OP(STOPROP)
        /* 0x08 */ OP(STOELT) OP(APPENDELT) OP(APPENDPROP) OP(LOADTRUE)
        /* 0x0C */ OP(LOADFALSE) OP(LOADNULL) OP(PUSH) OP(POP)

        /* 0x10 */ OP(LOR) OP(LAND) OP(OR) OP(AND)
        /* 0x14 */ OP(XOR) OP(EQ) OP(NE) OP(LT)
        /* 0x18 */ OP(LE) OP(GT) OP(GE) OP(SHL)
        /* 0x1C */ OP(SHR) OP(SAR) OP(ADD) OP(SUB)
        
        /* 0x20 */ OP(MUL)  OP(DIV)  OP(MOD)  OP(LINENO)
        /* 0x24 */ OP(LOADTHIS)  OP(LOADUP)  OP(STOREUP)  OP(CLOSURE)
        /* 0x28 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x2c */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x30 */ OP(UMINUS)  OP(UNOT)  OP(UNEG)  OP(PREINC)
        /* 0x34 */ OP(PREDEC)  OP(POSTINC)  OP(POSTDEC)  OP(CALL)
        /* 0x38 */ OP(NEW)  OP(CALLPROP) OP(JMP)  OP(JT)
        /* 0x3c */ OP(JF) OP(END) OP(RET) OP(UNKNOWN)
    };
    
static_assert (sizeof(dispatchTable) == 64 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        inst = code[i++]; \
        op = static_cast<Op>(inst.op()); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    if (!func) {
        return String();
    }
    
    assert(func->isFunction());
    const Function* function = static_cast<const Function*>(func);
    
    m8r::String outputString;

    _nestingLevel = nestingLevel;
	
	m8r::String name;
    if (functionName[0] != '\0') {
        name = "<";
        name += functionName;
        name += ">";
    }
    else {
        name = "anonymous>";
    }
    
    indentCode(outputString);
    outputString += "FUNCTION(";
    outputString += name.c_str();
    outputString += ")\n";
    
    _nestingLevel++;
    
    const Instruction* code = &(function->code()->at(0));

    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    uint32_t i = 0;
    for (uint32_t i = 0; ; ++i) {
        if (i >= function->code()->size()) {
            outputString += "\n\nWENT PAST THE END OF CODE\n\n";
            return outputString;
        }

        Instruction c = function->code()->at(i);
        Op op = static_cast<Op>(c.op());
        if (op == Op::END) {
            break;
        }

        if (op == Op::JT || op == Op::JF || op == Op::JMP) {
            int16_t addr = c.sn();
            Annotation annotation = { static_cast<uint32_t>(i + addr), uniqueID++ };
            annotations.push_back(annotation);
        }
    }
    
    // Display the constants and up values
    // We don't show the first Constant, it is a dummy error value
    if (function->constantCount() > 1) {
        indentCode(outputString);
        outputString += "CONSTANTS:\n";
        _nestingLevel++;

        for (uint8_t i = 1; i < function->constantCount(); ++i) {
            Value constant = function->constant(ConstantId(i));
            indentCode(outputString);
            outputString += "[" + Value::toString(i) + "] = ";
            showValue(program, outputString, constant);
            outputString += "\n";
        }
        _nestingLevel--;
        outputString += "\n";
    }
    
    if (function->upValueCount()) {
        indentCode(outputString);
        outputString += "UPVALUES:\n";
        _nestingLevel++;

        for (uint8_t i = 0; i < function->upValueCount(); ++i) {
            indentCode(outputString);
            outputString += "[" + Value::toString(i) + "] = ";
            uint32_t index;
            uint16_t frame;
            Atom name;
            function->upValue(i, index, frame, name);
            outputString += String("UP('") + program->stringFromAtom(name).c_str() + "':index=" + Value::toString(index) + ", frame=" + Value::toString(frame) + ")\n";
        }
        _nestingLevel--;
        outputString += "\n";
    }

    indentCode(outputString);
    outputString += "CODE:\n";
    _nestingLevel++;

    i = 0;
    m8r::String strValue;
    Atom localName;
    
    Instruction inst;
    Op op;
    
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_END:
        _nestingLevel--;
        preamble(outputString, i - 1);
        outputString += "END\n";
        _nestingLevel--;
        return outputString;  
    L_LOADLITA:
    L_LOADLITO:
    L_LOADTRUE:
    L_LOADFALSE:
    L_LOADNULL:
    L_LOADTHIS:
        generateRXX(outputString, i - 1, op, inst.ra());
        DISPATCH;
    L_PUSH:
        generateRXX(outputString, i - 1, op, inst.rn());
        DISPATCH;
    L_POP:
        generateRXX(outputString, i - 1, op, inst.ra());
        DISPATCH;
    L_MOVE: L_LOADREFK:
    L_UMINUS: L_UNOT: L_UNEG: L_CLOSURE:
    L_PREINC: L_PREDEC: L_POSTINC: L_POSTDEC:
    L_APPENDELT:
        generateRRX(outputString, i - 1, op, inst.ra(), inst.rb());
        DISPATCH;
    L_LOADUP:
        generateRUX(outputString, i - 1, op, inst.ra(), inst.rb());
        DISPATCH;
    L_STOREUP:
        generateURX(outputString, i - 1, op, inst.ra(), inst.rb());
        DISPATCH;
    L_STOREFK:
        generateRRX(outputString, i - 1, op, inst.rb(), inst.rc());
        DISPATCH;
    L_LOADPROP: L_LOADELT: L_STOPROP: L_STOELT: L_APPENDPROP:
    L_LOR: L_LAND: L_OR: L_AND: L_XOR:
    L_EQ: L_NE: L_LT: L_LE: L_GT: L_GE:
    L_SHL: L_SHR: L_SAR:
    L_ADD: L_SUB: L_MUL: L_DIV: L_MOD:
    L_DEREF:
        generateRRR(outputString, i - 1, op, inst.ra(), inst.rb(), inst.rc());
        DISPATCH;
    L_RET:
        generateXN(outputString, i - 1, op, inst.nparams());
        DISPATCH;
    L_JMP:
        generateXN(outputString, i - 1, op, inst.sn());
        DISPATCH;
    L_JT: L_JF:
        generateRN(outputString, i - 1, op, inst.rn(), inst.sn());
        DISPATCH;
    L_CALL:
        generateCall(outputString, i - 1, op, inst.rcall(), inst.rthis(), inst.nparams());
        DISPATCH;
    L_NEW:
        generateRN(outputString, i - 1, op, inst.rcall(), inst.nparams());
        DISPATCH;
    L_CALLPROP:
        generateCall(outputString, i - 1, op, inst.rcall(), inst.rthis(), inst.nparams());
        DISPATCH;
    L_LINENO:
        _lineno = inst.un();
        DISPATCH;
}

struct CodeMap
{
    Op op;
    const char* s;
};

#undef OP
#define OP(op) { Op::op, #op },

static CodeMap opcodes[] = {    
    OP(RET)
    OP(END)
    
    OP(MOVE) OP(LOADREFK) OP(STOREFK) OP(LOADLITA) OP(LOADLITO)
    OP(LOADPROP) OP(LOADELT) OP(STOPROP) OP(STOELT) OP(APPENDELT) OP(APPENDPROP)
    OP(LOADTRUE) OP(LOADFALSE) OP(LOADNULL) OP(LOADTHIS) OP(LOADUP) OP(STOREUP)
    OP(PUSH) OP(POP)
    
    OP(LOR) OP(LAND) OP(OR) OP(AND) OP(XOR)
    OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE)
    OP(SHL) OP(SHR) OP(SAR) 
    OP(ADD) OP(SUB) OP(MUL) OP(DIV) OP(MOD) 
    
    OP(UMINUS) OP(UNOT) OP(UNEG) OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) 
    
    OP(CALL) OP(NEW) OP(CALLPROP) OP(JMP) OP(JT) OP(JF) OP(LINENO) OP(CLOSURE)
};

const char* CodePrinter::stringFromOp(Op op)
{
    for (auto c : opcodes) {
        if (c.op == op) {
            return c.s;
        }
    }
    return "UNKNOWN";
}

void CodePrinter::indentCode(m8r::String& s) const
{
    for (uint32_t i = 0; i < _nestingLevel; ++i) {
        s += "    ";
    }
}

void CodePrinter::showValue(const Program* program, m8r::String& s, const Value& value) const
{
    switch(value.type()) {
        case Value::Type::NativeObject: s += "NativeObject"; break;
        case Value::Type::Object: s += "Object"; break;
        case Value::Type::None: s += "NONE"; break;
        case Value::Type::Null: s += "Null"; break;
        case Value::Type::Float: s += "FLT(" + Value::toString(value.asFloatValue()) + ")"; break;
        case Value::Type::Integer: s += "INT(" + Value::toString(value.asIntValue()) + ")"; break;
        case Value::Type::String: s += "***String***"; break;
        case Value::Type::StringLiteral: s += "STR(\"" + String(program->stringFromStringLiteral(value.asStringLiteralValue())) + "\")"; break;
        case Value::Type::Id: s += "ATOM(\"" + program->stringFromAtom(value.asIdValue()) + "\")"; break;
        case Value::Type::Function: {
            Function* func = value.asFunction();
            if (func) {
                _nestingLevel++;
                s += "\n";
                String name = func->name() ? program->stringFromAtom(func->name()) : String("unnamed");
                s += generateCodeString(program, func, name.c_str(), _nestingLevel);
                _nestingLevel--;
                break;
            }
            // FIXME: Show the address of the obj or something
            s += "OBJ";
            break;
        }
    }
}
