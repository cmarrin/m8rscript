/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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

void CodePrinter::preamble(String& s, uint32_t addr, bool indent) const
{
    if (_lineno != _emittedLineNumber) {
        _emittedLineNumber = _lineno;
        if (_nestingLevel) {
            --_nestingLevel;
            indentCode(s);
            ++_nestingLevel;
        }
        s += "LINENO[";
        s += String(_lineno);
        s += "]\n";
    }
    
    uint32_t uniqueID = findAnnotation(addr);
    if (!uniqueID) {
        if (indent) {
            indentCode(s);
        }
        return;
    }
    if (_nestingLevel) {
        --_nestingLevel;
        indentCode(s);
        ++_nestingLevel;
    }
    s += "LABEL[";
    s += String(uniqueID);
    s += "]\n";
    if (indent) {
        indentCode(s);
    }
}

uint8_t CodePrinter::regOrConst(const Mad<Object> func, const uint8_t*& code, Value& constant) const
{
    uint32_t r = byteFromCode(code);
    if (r <= MaxRegister) {
        return r;
    }

    if (shortSharedAtomConstant(r)) {
        constant = Value(Atom(byteFromCode(code)));
    } else if (longSharedAtomConstant(r)) {
        constant = Value(Atom(uNFromCode(code)));
    } else {
        func->constant(r, constant);
    }
    return r;
}

m8r::String CodePrinter::generateCodeString(const Mad<Program> program) const
{    
    return generateCodeString(program, program, "main", 0);
}

String CodePrinter::regString(const Mad<Program> program, const Mad<Object> function, const uint8_t*& code, bool up) const
{
    Value constant;
    uint8_t r = regOrConst(function, code, constant);
    return regString(program, function, r, constant, up);
}

String CodePrinter::regString(const Mad<Program> program, const Mad<Object> function, uint8_t reg, const Value& constant, bool up) const
{
    if (up) {
        return String("U[") + String(reg) + "]";
    }
    if (reg <= MaxRegister) {
        return String("R[") + String(reg) + "]";
    }
    
    reg -= MaxRegister + 1;
    
    String s = String("K[");
    
    if (reg < builtinConstantOffset()) {
        showConstant(program, s, constant, true);
        s += ']';
    } else {
        s += String(reg - builtinConstantOffset()) + "](";
        showConstant(program, s, constant, true);
        s += ")";
    }
    return s;
}

void CodePrinter::generateXXX(m8r::String& str, uint32_t addr, Op op) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + "\n";
}

void CodePrinter::generateRXX(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code) + "\n";
}

void CodePrinter::generateRRX(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code) + ", " + regString(program, function, code) + "\n";
}

void CodePrinter::generateRUX(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code) + ", " + regString(program, function, code, true) + "\n";
}

void CodePrinter::generateURX(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code, true) + ", " + regString(program, function, code) + "\n";
}

void CodePrinter::generateRRR(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code) + ", " + regString(program, function, code) + ", " + regString(program, function, code) + "\n";
}

void CodePrinter::generateRParams(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code) + ", " + String(byteFromCode(code)) + "\n";
}

void CodePrinter::generateRRParams(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    str += String(stringFromOp(op)) + " " + regString(program, function, code) + ", " + regString(program, function, code) + ", " + String(byteFromCode(code)) + "\n";
}

void CodePrinter::generateJumpAddr(m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    int16_t targetAddr = sNFromCode(code);
    uint32_t id = findAnnotation(static_cast<uint32_t>(addr + targetAddr));
    str += String(stringFromOp(op)) + " " + ((id == 0) ? "[???]" : (String("LABEL[") + String(id) + "]")) + "\n";
}

void CodePrinter::generateRJumpAddr(const Mad<Program> program, const Mad<Object> function, m8r::String& str, uint32_t addr, Op op, const uint8_t*& code) const
{
    preamble(str, addr);
    String regstr = regString(program, function, code);
    int16_t targetAddr = sNFromCode(code);
    uint32_t id = findAnnotation(static_cast<uint32_t>(addr + targetAddr));
    str += String(stringFromOp(op)) + " " + regstr + ", " + ((id == 0) ? "[???]" : (String("LABEL[") + String(id) + "]")) + "\n";
}

m8r::String CodePrinter::generateCodeString(const Mad<Program> program, const Mad<Function> func, const char* functionName, uint32_t nestingLevel) const
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
        
        /* 0x20 */ OP(MUL)  OP(DIV)  OP(MOD)  OP(UMINUS)
        /* 0x24 */ OP(UNOT)  OP(UNEG)  OP(PREINC)  OP(PREDEC)
        /* 0x28 */ OP(POSTINC)  OP(POSTDEC)  OP(CALL)  OP(NEW)
        /* 0x2c */ OP(CALLPROP) OP(JMP)  OP(JT)  OP(JF)

        /* 0x30 */ OP(LINENO)  OP(LOADTHIS)  OP(LOADUP)  OP(CLOSURE)
        /* 0x34 */ OP(UNKNOWN) OP(POPX)  OP(RETI)  OP(UNKNOWN)
        /* 0x38 */ OP(UNKNOWN) OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x3c */ OP(UNKNOWN) OP(END) OP(RET) OP(UNKNOWN)
    };
    
static_assert (sizeof(dispatchTable) == 64 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = opFromCode(currentAddr, imm); \
        pc = static_cast<uint32_t>(currentAddr - code - 1); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    if (!func.valid()) {
        return String();
    }
        
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

    const uint8_t* code = &(func->code()->at(0));
    Op op;
    uint8_t imm;
    
    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    const uint8_t* end = code + func->code()->size();
    
    for (const uint8_t* p = code; ; ) {
        if (p >= end) {
            outputString += "\n\nWENT PAST THE END OF CODE\n\n";
            return outputString;
        }

        const uint8_t* jumpAddr = p;
        
        op = opFromCode(p, imm);
        if (op == Op::END) {
            break;
        }

        if (op == Op::JT || op == Op::JF || op == Op::JMP) {
            if (op == Op::JT || op == Op::JF) {
                p += constantSize(byteFromCode(p));
            }

            uint32_t addr = static_cast<uint32_t>((jumpAddr - code) + sNFromCode(p));
            Annotation annotation = { addr, uniqueID++ };
            annotations.push_back(annotation);
        } else {
            if (OpInfo::aReg(op)) {
                p += constantSize(byteFromCode(p));
            }
            if (OpInfo::bReg(op)) {
                p += constantSize(byteFromCode(p));
            }
            if (OpInfo::cReg(op)) {
                p += constantSize(byteFromCode(p));
            }
            if (OpInfo::dReg(op)) {
                p += constantSize(byteFromCode(p));
            }
            if (OpInfo::params(op)) {
                p++;
            }
            if (OpInfo::number(op)) {
                p += 2;
            }
        }
    }
    
    // Display the constants and up values
    // We don't show the first Constant, it is a dummy error value
    bool first = true;
    func->enumerateConstants([&](const Value& value, const ConstantId& id) {
        if (first) {
            indentCode(outputString);
            outputString += "CONSTANTS:\n";
            _nestingLevel++;
            first = false;
        }
        
        indentCode(outputString);
        outputString += "[" + String(id.raw()) + "] = ";
        showConstant(program, outputString, value);
        outputString += "\n";
    });
    
    if (!first) {
        _nestingLevel--;
        outputString += "\n";
    }
    
    if (func->upValueCount()) {
        indentCode(outputString);
        outputString += "UPVALUES:\n";
        _nestingLevel++;

        for (uint8_t i = 0; i < func->upValueCount(); ++i) {
            indentCode(outputString);
            outputString += "[" + String(i) + "] = ";
            uint32_t index;
            uint16_t frame;
            Atom name;
            func->upValue(i, index, frame, name);
            outputString += String("UP('") + program->stringFromAtom(name) + "':index=" + String(index) + ", frame=" + String(frame) + ")\n";
        }
        _nestingLevel--;
        outputString += "\n";
    }

    indentCode(outputString);
    outputString += "CODE:\n";
    _nestingLevel++;

    m8r::String strValue;
    Atom localName;
    
    const uint8_t* currentAddr = code;
    uint32_t pc = 0;
    op = Op::UNKNOWN;
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_END:
        preamble(outputString, pc, false);
        _nestingLevel--;
        indentCode(outputString);
        outputString += "END\n";
        _nestingLevel--;
        return outputString;
    L_LOADLITA:
    L_LOADLITO:
    L_LOADTRUE:
    L_LOADFALSE:
    L_LOADNULL:
    L_LOADTHIS:
    L_PUSH:
    L_POP:
        generateRXX(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_POPX:
        generateXXX(outputString, pc, op);
        DISPATCH;
    L_MOVE: L_LOADREFK: L_STOREFK:
    L_APPENDELT:
    L_UMINUS: L_UNOT: L_UNEG:
    L_PREINC: L_PREDEC: L_POSTINC: L_POSTDEC:
    L_CLOSURE:
        generateRRX(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_LOADUP:
        generateRUX(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_LOADPROP: L_LOADELT: L_STOPROP: L_STOELT: L_APPENDPROP:
    L_LOR: L_LAND: L_OR: L_AND: L_XOR:
    L_EQ: L_NE: L_LT: L_LE: L_GT: L_GE:
    L_SHL: L_SHR: L_SAR:
    L_ADD: L_SUB: L_MUL: L_DIV: L_MOD:
        generateRRR(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_RET:
        preamble(outputString, pc);
        outputString += String(stringFromOp(op)) + " " + String(byteFromCode(currentAddr)) + "\n";
        DISPATCH;
    L_RETI:
        preamble(outputString, pc);
        outputString += String(stringFromOp(op)) + " " + String(imm) + "\n";
        DISPATCH;
    L_JMP:
    {
        generateJumpAddr(outputString, pc, op, currentAddr);
        DISPATCH;
    }
    L_JT: L_JF:
    {
        generateRJumpAddr(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    }
    L_CALL:
        generateRRParams(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_NEW:
        generateRParams(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_CALLPROP:
        generateRRParams(program, func, outputString, pc, op, currentAddr);
        DISPATCH;
    L_LINENO:
        _lineno = uNFromCode(currentAddr);
        if (findAnnotation(pc)) {
            preamble(outputString, pc, false);
        }
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
    OP(MOVE) OP(LOADREFK) OP(STOREFK) OP(LOADLITA) 
    OP(LOADLITO) OP(LOADPROP) OP(LOADELT) OP(STOPROP) 
    OP(STOELT) OP(APPENDELT) OP(APPENDPROP) OP(LOADTRUE)
    OP(LOADFALSE) OP(LOADNULL) OP(PUSH) OP(POP) 
    
    OP(LOR) OP(LAND) OP(OR) OP(AND) 
    OP(XOR) OP(EQ) OP(NE) OP(LT) 
    OP(LE) OP(GT) OP(GE) OP(SHL)
    OP(SHR) OP(SAR) OP(ADD) OP(SUB) 
    
    OP(MUL) OP(DIV) OP(MOD) OP(UMINUS)
    OP(UNOT) OP(UNEG) OP(PREINC) OP(PREDEC)
    OP(POSTINC) OP(POSTDEC) OP(CALL) OP(NEW) 
    OP(CALLPROP) OP(JMP) OP(JT) OP(JF) 
    
    OP(LINENO) OP(LOADTHIS) OP(LOADUP)
    OP(CLOSURE) OP(POPX) OP(RETI)
    
    OP(END) OP(RET)
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

static String escapeString(const String& s)
{
    Vector<String> array = s.split("\n");
    return String::join(array, "\\n");
}

void CodePrinter::showConstant(const Mad<Program> program, m8r::String& s, const Value& value, bool abbreviated) const
{
    switch(value.type()) {
        case Value::Type::StaticObject: s += "StaticObject"; break;
        case Value::Type::NativeFunction: s += "NativeFunction"; break;
        case Value::Type::NativeObject: s += "NativeObject"; break;
        case Value::Type::Undefined: s += "Undefined"; break;
        case Value::Type::Null: s += "Null"; break;
        case Value::Type::Float: s += "FLT(" + String(value.asFloatValue()) + ")"; break;
        case Value::Type::Integer: s += "INT(" + String(value.asIntValue()) + ")"; break;
        case Value::Type::String: s += "***String***"; break;
        case Value::Type::StringLiteral: {
            String lit = String(program->stringFromStringLiteral(value.asStringLiteralValue()));
            lit = escapeString(lit);
            if (abbreviated) {
                lit = lit.slice(0, 10) + ((lit.size() > 10) ? "..." : "");
            }
            s += "STR(\"" + lit + "\")";
            break;
        }
        case Value::Type::Id: 
            s += "Atom(\""; 
            s += program->stringFromAtom(value.asIdValue()); 
            s += "\")"; 
            break;
        case Value::Type::Object: {
            Mad<Object> obj = value.asObject();
            if (!obj.valid()) {
                break;
            }
            if (obj->code()) {
                if (abbreviated) {
                    s += "FUNCTION";
                    break;
                }
                _nestingLevel++;
                s += "\n";
                String name = obj->name() ? program->stringFromAtom(obj->name()) : String("unnamed");
                s += generateCodeString(program, Mad<Function>(obj.raw()), name.c_str(), _nestingLevel);
                _nestingLevel--;
                break;
            }
            
            if (abbreviated) {
                s += "CLASS";
                break;
            }
            
            if (obj.valid()) {
                _nestingLevel++;
                s += "CLASS {\n";
                uint32_t count = obj->numProperties();

                for (int32_t i = 0; i < count; ++i) {
                    Atom name = obj->propertyKeyforIndex(i);
                    if (name) {
                        Value v = obj->property(name);
                        indentCode(s);
                        s += program->stringFromAtom(name);
                        s += " : ";
                        showConstant(program, s, v);
                        s += "\n";
                    }
                }
                _nestingLevel--;
                indentCode(s);
                s += "}";
            }
            break;
        }
    }
}
