/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Defines.h"
#ifndef SCRIPT_SUPPORT
static_assert(0, "SCRIPT_SUPPORT not defined");
#endif
#if SCRIPT_SUPPORT == 1

#include "Parser.h"

#include "ParseEngine.h"
#include "ExecutionUnit.h"
#include "GC.h"
#include <limits>

using namespace m8r;

uint32_t Parser::_nextLabelId = 1;

Parser::Parser(Mad<Program> program)
    : _parseStack(this)
    , _program(program.valid() ? program : Object::create<Program>())
{
    // While parsing the program is unprotected. It could get collected.
    // Register it to protect it during the compile
    GC::addStaticObject(_program.raw());
}

Parser::~Parser()
{
    GC::removeStaticObject(_program.raw());
}

Mad<Function> Parser::parse(const m8r::Stream& stream, ExecutionUnit* eu, Debug debug, Mad<Function> parent)
{
    assert(eu);
    _eu = eu;
    _debug = debug;
    _scanner.setStream(&stream);
    ParseEngine p(this);
    if (!parent.valid()) {
        parent = _program;
    }
    _functions.emplace_back(parent, false);
    p.program();
    return functionEnd();
}

void Parser::printError(ROMString format, ...)
{
    va_list args;
    va_start(args, format);
    _eu->printf(ROMSTR("***** "));
    _eu->print(Error::vformatError(Error::Code::ParseError, _scanner.lineno(), format, args).c_str());
    va_end(args);
    
    va_start(args, format);
    String s = ROMString::vformat(format, args);
    _syntaxErrors.emplace_back(s.c_str(), _scanner.lineno(), 1, 1);
    va_end(args);
}

void Parser::unknownError(Token token)
{
    uint8_t c = static_cast<uint8_t>(token);
    printError(ROMSTR("unknown token (%s)"), String(c).c_str());
}

void Parser::expectedError(Token token, const char* s)
{
    char c = static_cast<char>(token);
    if (c >= 0x20 && c <= 0x7f) {
        printError(ROMSTR("syntax error: expected '%c'"), c);
    } else {
        switch(token) {
            case Token::DuplicateDefault: printError(ROMSTR("multiple default cases not allowed")); break;
            case Token::Expr: assert(s); printError(ROMSTR("expected %s%sexpression"), s ?: "", s ? " " : ""); break;
            case Token::PropertyAssignment: printError(ROMSTR("expected object member")); break;
            case Token::Statement: printError(ROMSTR("statement expected")); break;
            case Token::Identifier: printError(ROMSTR("identifier")); break;
            case Token::MissingVarDecl: printError(ROMSTR("missing var declaration")); break;
            case Token::OneVarDeclAllowed: printError(ROMSTR("only one var declaration allowed here")); break;
            case Token::ConstantValueRequired: printError(ROMSTR("constant value required")); break;
            case Token::EndOfFile: printError(ROMSTR("unable to continue parsing")); break;
            default: printError(ROMSTR("*** UNKNOWN TOKEN ***")); break;
        }
    }
}

Label Parser::label()
{
    Label label;
    if (!nerrors()) {
        label.label = static_cast<int32_t>(_deferred ? _deferredCode.size() : currentCode().size());
        label.uniqueID = _nextLabelId++;
    }
    return label;
}

void Parser::addMatchedJump(Op op, Label& label)
{
    if (nerrors()) return;
    
    assert(op == Op::JMP || op == Op::JT || op == Op::JF);

    RegOrConst reg;
    if (op != Op::JMP) {
        reg = _parseStack.bake();
        _parseStack.pop();
    }
    // Emit opcode with a dummy address
    label.matchedAddr = static_cast<int32_t>(_deferred ? _deferredCode.size() : currentCode().size());
    if (op != Op::JMP) {
        emitCode(op, reg, static_cast<int16_t>(0));
    } else {
        emitCode(op, static_cast<int16_t>(0));
    }
}

void Parser::doMatchJump(int32_t matchAddr, int32_t jumpAddr)
{
    if (nerrors()) return;
    
    if (jumpAddr < -MaxJump || jumpAddr > MaxJump) {
        printError(ROMSTR("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n"));
        return;
    }
    
    Op op = opFromByte(_deferred ? _deferredCode.at(matchAddr) : currentCode().at(matchAddr));
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    int16_t emitAddr = (op == Op::JMP) ? (matchAddr + 1) : (matchAddr + 2);
    uint8_t* code = _deferred ? &(_deferredCode[emitAddr]) : &(currentCode()[emitAddr]);
    code[0] = static_cast<uint8_t>(jumpAddr >> 8);
    code[1] = static_cast<uint8_t>(jumpAddr);
}

void Parser::jumpToLabel(Op op, Label& label)
{
    if (nerrors()) return;
    
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    int32_t jumpAddr = label.label - static_cast<int32_t>(_deferred ? _deferredCode.size() : currentCode().size());

    if (jumpAddr < -MaxJump || jumpAddr > MaxJump) {
        printError(ROMSTR("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n"));
    }
    
    RegOrConst r;
    if (op != Op::JMP) {
        r = _parseStack.bake();
        _parseStack.pop();
    }

    if (op == Op::JMP) {
        emitCode(op, static_cast<int16_t>(jumpAddr));
    } else {
        emitCode(op, r, static_cast<int16_t>(jumpAddr));
    }
}

void Parser::addCode(Op op, RegOrConst reg0, RegOrConst reg1, RegOrConst reg2, uint16_t n)
{
    if (op != Op::LINENO && op != Op::JF && op != Op::JT && op != Op::JMP) {
        emitLineNumber();
    }
    
    Vector<uint8_t>* vec;
    if (_deferred) {
        assert(_deferredCodeBlocks.size() > 0);
        vec = &_deferredCode;
    } else {
        vec = &currentCode();
    }
    
    if (op == Op::RET && n <= 3) {
        op = Op::RETI;
        vec->push_back(static_cast<uint8_t>(op) | n << 6);
    } else {
        vec->push_back(static_cast<uint8_t>(op));
    }
    
    RegOrConst regs[4] = { reg0, reg1, reg2 };
    uint8_t regIndex = 0;
    
    // aReg and dReg must be regs, bReg and cReg can be reg or constant
    if (OpInfo::aReg(op)) {
        assert(regs[regIndex].isReg());
        regs[regIndex++].push(vec);
    }

    if (OpInfo::bReg(op)) {
        regs[regIndex++].push(vec);
    }

    if (OpInfo::cReg(op)) {
        regs[regIndex++].push(vec);
    }
    
    if (OpInfo::dReg(op)) {
        assert(regs[regIndex].isReg());
        assert(regIndex < 3);
        regs[regIndex++].push(vec);
    }
    
    assert(!OpInfo::number(op) || !OpInfo::params(op));

    if (OpInfo::number(op)) {
        vec->push_back(static_cast<uint8_t>(n >> 8));
        vec->push_back(static_cast<uint8_t>(n));
    }
    
    if (OpInfo::params(op)) {
        vec->push_back(static_cast<uint8_t>(n));
    }

    if (op == Op::JF || op == Op::JT || op == Op::JMP) {
        emitLineNumber();
    }
}

Parser::RegOrConst Parser::addConstant(const Value& v)
{
    if (currentConstants().size() >= std::numeric_limits<uint8_t>::max()) {
        printError(ROMSTR("TOO MANY CONSTANTS IN FUNCTION!\n"));
        return RegOrConst();
    }
    
    // See if it's a Builtin
    switch (v.type()) {
        case Value::Type::Id: {
            ConstantId id = ConstantId(static_cast<ConstantId::value_type>((v.asIdValue().raw() < 256) ? BuiltinConstants::AtomShort : BuiltinConstants::AtomLong));
            return RegOrConst(id, v.asIdValue());
        }
        case Value::Type::Null:
            return RegOrConst(ConstantId(static_cast<ConstantId::value_type>(BuiltinConstants::Null)));
        case Value::Type::Undefined:
            return RegOrConst();
        case Value::Type::Integer:
            if (v.asIntValue() == 0) {
                return RegOrConst(ConstantId(static_cast<ConstantId::value_type>(BuiltinConstants::Int0)));
            } else if (v.asIntValue() == 1) {
                return RegOrConst(ConstantId(static_cast<ConstantId::value_type>(BuiltinConstants::Int1)));
            } else {
                break;
            }
        default: break;
    }
    
    for (ConstantId::value_type id = 0; id < currentConstants().size(); ++id) {
        if (currentConstants()[id] == v) {
            return RegOrConst(ConstantId(id + builtinConstantOffset()));
        }
    }
    
    ConstantId r(static_cast<ConstantId::value_type>(currentConstants().size() + builtinConstantOffset()));
    currentConstants().push_back(v);
    return RegOrConst(r);
}

void Parser::pushK(const char* s)
{
    if (nerrors()) return;
    
    _parseStack.pushConstant(addConstant(Value(_program->addStringLiteral(s))));
}

void Parser::pushK(const Value& value)
{
    if (nerrors()) return;
    
    _parseStack.pushConstant(addConstant(value));
}

void Parser::addNamedFunction(Mad<Function> func, const Atom& name)
{
    if (nerrors()) return;

    addConstant(Value(func));
    func->setName(name);
}

void Parser::pushThis()
{
    if (nerrors()) return;
    _parseStack.push(ParseStack::Type::This, RegOrConst());
}

void Parser::pushTmp()
{
    if (nerrors()) return;
    _parseStack.pushRegister();
}

void Parser::emitId(const char* s, IdType type)
{
    if (nerrors()) return;
    
    emitId(_program->atomizeString(s), type);
}

void Parser::emitId(const Atom& atom, IdType type)
{
    if (nerrors()) return;
    
    if (type == IdType::MightBeLocal || type == IdType::MustBeLocal) {
        // See if it's a local function
        for (uint32_t i = 0; i < currentConstants().size(); ++i) {
            Mad<Object> func = currentConstants().at(ConstantId(i).raw()).asObject();
            if (func.valid()) {
                if (func->name() == atom) {
                    _parseStack.pushConstant(RegOrConst(ConstantId(i + builtinConstantOffset())));
                    return;
                }
            }
        }
        
        // Find the id in the function chain
        bool local = true;
        uint16_t frame = 0;
        for (int32_t i = static_cast<int32_t>(_functions.size()) - 1; i >= 0; --i) {
            int32_t index = _functions[i].localIndex(atom);
            
            if (index >= 0) {
                if (local) {
                    _parseStack.push(ParseStack::Type::Local, RegOrConst(static_cast<uint32_t>(index)));
                    return;
                }
                
                _parseStack.push(ParseStack::Type::UpValue, RegOrConst(static_cast<uint32_t>(currentFunction()->addUpValue(index, frame, atom))));
                return;
            }
            local = false;
            frame++;
            
            if (type == IdType::MustBeLocal) {
                String s = "nonexistent variable '";
                s += _program->stringFromAtom(atom);
                s += "'";
                printError(ROMSTR("%s"), s.c_str());
                return;
            }
        }
    }
    
    if (atom == Atom(SA::value)) {
        _parseStack.push((type == IdType::NotLocal) ? ParseStack::Type::Constant : ParseStack::Type::RefK, RegOrConst());
        _parseStack.setIsValue(true);
        return;
    }
    
    RegOrConst id = addConstant(Value(atom));
    _parseStack.push((type == IdType::NotLocal) ? ParseStack::Type::Constant : ParseStack::Type::RefK, id);
}

void Parser::emitMove()
{
    if (nerrors()) return;
    
    // RefK needs to do a STOREFK TOS-1, TOS and then leave TOS-1 (the source) on the stack
    // All others need to do a save and leave TOS (the dest) on the stack
    RegOrConst srcReg = _parseStack.bake(true);
    _parseStack.swap();
    ParseStack::Type dstType = _parseStack.topType();
    
    switch(dstType) {
        case ParseStack::Type::This:
            printError(ROMSTR("Assignment to 'this' not allowed\n"));
            break;
        case ParseStack::Type::Unknown:
        case ParseStack::Type::Constant:
            assert(0);
            break;
        case ParseStack::Type::PropRef:
        case ParseStack::Type::EltRef: {
            if (_parseStack.topIsValue()) {
                // Currently TOS is a PropRef and TOS-1 is the source. We need to convert the
                // PropRef into a simple register, then swap it back to its original position,
                // then push a "setValue" atom, the swap again. Then we will have:
                // TOS=>src, Atom(setValue), dst. Now we need to generate the equivalent of:
                //
                //      dst.setValue(src
                //
                _parseStack.propRefToReg();
                _parseStack.swap();
                emitId(Atom(SA::setValue), IdType::NotLocal);
                _parseStack.swap();
                emitPush();
                RegOrConst objReg = emitDeref(DerefType::Prop);
                emitCallRet(Op::CALL, objReg, 1);
                discardResult();
                return;
            } else {
                emitCode((dstType == ParseStack::Type::PropRef) ? Op::STOPROP : Op::STOELT, _parseStack.topReg(), _parseStack.topDerefReg(), srcReg);
            }
            break;
        case ParseStack::Type::Local:
        case ParseStack::Type::Register:
            emitCode(Op::MOVE, _parseStack.topReg(), srcReg);
            break;
        case ParseStack::Type::RefK:
            emitCode(Op::STOREFK, _parseStack.topReg(), srcReg);
            _parseStack.pop();
            assert(_parseStack.topReg() == srcReg);
            return;
        case ParseStack::Type::UpValue:
            printError(ROMSTR("assignment to up-value not allowed, use boxed value instead"));
            break;
        }
    }
    
    _parseStack.swap();
    _parseStack.pop();
}

Parser::RegOrConst Parser::emitDeref(DerefType type)
{
    if (nerrors()) return RegOrConst();
    
    bool isValue = _parseStack.topIsValue();
    RegOrConst derefReg = _parseStack.bake();
    _parseStack.swap();
    RegOrConst objectReg = _parseStack.bake();
    _parseStack.swap();
    _parseStack.pop();
    _parseStack.replaceTop((type == DerefType::Prop) ? ParseStack::Type::PropRef : ParseStack::Type::EltRef, objectReg, derefReg);
    _parseStack.setIsValue(isValue);
    return objectReg;
}

void Parser::emitDup()
{
    if (nerrors()) return;
    
    switch(_parseStack.topType()) {
        case ParseStack::Type::PropRef:
        case ParseStack::Type::EltRef:
        case ParseStack::Type::RefK:
        case ParseStack::Type::Constant:
            _parseStack.dup();
            _parseStack.bake();
            break;
        case ParseStack::Type::Local:
            _parseStack.push(ParseStack::Type::Local, _parseStack.topReg());
            break;
        case ParseStack::Type::UpValue:
            _parseStack.push(ParseStack::Type::UpValue, _parseStack.topReg());
            break;
        default:
            assert(0);
            break;
    }
}

void Parser::emitAppendElt()
{
    if (nerrors()) return;
    
    // tos-1 object to append to
    // tos value to store
    // leave object on tos
    RegOrConst srcReg = _parseStack.bake();
    _parseStack.swap();
    RegOrConst objectReg = _parseStack.bake();
    _parseStack.swap();
    _parseStack.pop();
    
    emitCode(Op::APPENDELT, objectReg, srcReg);
}

void Parser::emitAppendProp()
{
    if (nerrors()) return;
    
    // tos-2 object to append to
    // tos-1 property on that object
    // tos value to store
    // leave object on tos
    RegOrConst srcReg = _parseStack.bake();
    _parseStack.swap();
    RegOrConst propReg = _parseStack.bake();
    _parseStack.pop();
    _parseStack.pop();
    assert(!_parseStack.needsBaking());
    RegOrConst objectReg = _parseStack.topReg();
    
    emitCode(Op::APPENDPROP, objectReg, propReg, srcReg);
}

void Parser::emitBinOp(Op op)
{
    if (nerrors()) return;
    
    if (op == Op::MOVE) {
        emitMove();
        return;
    }
    
    RegOrConst rightReg = _parseStack.bake();
    _parseStack.swap();
    RegOrConst leftReg = _parseStack.bake();
    _parseStack.pop();
    _parseStack.pop();
    RegOrConst dst = _parseStack.pushRegister();

    emitCode(op, dst, leftReg, rightReg);
}

void Parser::emitCaseTest()
{
    if (nerrors()) return;
    
    // This is like emitBinOp(Op::EQ), but does not pop the left operand
    RegOrConst rightReg = _parseStack.bake();
    _parseStack.swap();
    RegOrConst leftReg = _parseStack.bake();
    _parseStack.swap();
    _parseStack.pop();
    RegOrConst dst = _parseStack.pushRegister();

    emitCode(Op::EQ, dst, leftReg, rightReg);
}

void Parser::emitUnOp(Op op)
{
    if (nerrors()) return;
    
    if (op == Op::PREDEC || op == Op::PREINC || op == Op::POSTDEC || op == Op::POSTINC) {
        RegOrConst dst = _parseStack.pushRegister();
        _parseStack.swap();
        emitDup();
        RegOrConst srcReg = _parseStack.bake();
        emitCode(op, dst, srcReg);
        emitMove();
        _parseStack.pop();
        return;
    }
    
    RegOrConst srcReg = _parseStack.bake();
    _parseStack.pop();
    RegOrConst dst = _parseStack.pushRegister();
    emitCode(op, dst, srcReg);
}

void Parser::emitLoadLit(bool array)
{
    if (nerrors()) return;
    
    RegOrConst dst = _parseStack.pushRegister();
    emitCode(array ? Op::LOADLITA : Op::LOADLITO, dst);
}

void Parser::emitPush()
{
    if (nerrors()) return;
    
    RegOrConst src = _parseStack.bake(true);
    _parseStack.pop();
    
    emitCode(Op::PUSH, src);
}

void Parser::emitPop()
{
    if (nerrors()) return;
    
    RegOrConst dst = _parseStack.pushRegister();
    emitCode(Op::POP, dst);
}

void Parser::emitEnd()
{
    if (nerrors()) return;
    
    // If we have no errors we expect an empty stack, otherwise, there might be cruft left over
    if (nerrors()) {
        _parseStack.clear();
    }
    emitCode(Op::END);
}

void Parser::emitCallRet(Op op, RegOrConst thisReg, uint8_t nparams)
{
    if (nerrors()) return;
    
    assert(op == Op::CALL || op == Op::NEW || op == Op::RET);
    
    RegOrConst calleeReg = RegOrConst();
    if (thisReg == RegOrConst()) {
        // This uses a dummy value for this
        thisReg = RegOrConst();
    }

    if (op == Op::CALL) {
        // If tos is a PropRef or EltRef, emit CALLPROP with the object and property
        if (_parseStack.topType() == ParseStack::Type::PropRef || _parseStack.topType() == ParseStack::Type::EltRef) {
            emitCode(Op::CALLPROP, _parseStack.topReg(), _parseStack.topDerefReg(), nparams);
            _parseStack.pop();
            emitPop();
            return;
        }
    }
            
    
    if (op == Op::CALL || op == Op::NEW) {
        calleeReg = _parseStack.bake();
        _parseStack.pop();
    } else {
        // If there is a return value, push it onto the runtime stack
        for (uint8_t i = 0; i < nparams; ++i) {
            emitPush();
        }
    }
    
    if (op == Op::RET) {
        emitCode(Op::RET, nparams);
    } else if (op == Op::NEW) {
        emitCode(op, calleeReg, nparams);
    } else {
        emitCode(op, calleeReg, thisReg, nparams);
    }
    
    if (op == Op::CALL || op == Op::NEW) {
        // On return there will be a value on the runtime stack. Pop it into a register
        emitPop();
    }
}

int32_t Parser::emitDeferred()
{
    if (nerrors()) return 0;
    
    assert(!_deferred);
    assert(_deferredCodeBlocks.size() > 0);
    int32_t start = static_cast<int32_t>(currentCode().size());
    
    for (size_t i = _deferredCodeBlocks.back(); i < _deferredCode.size(); ++i) {
        currentCode().push_back(_deferredCode[i]);
    }
    _deferredCode.resize(_deferredCodeBlocks.back());
    _deferredCodeBlocks.pop_back();
    return start;
}

void Parser::discardResult()
{
    _parseStack.pop();
}

void Parser::functionAddParam(const Atom& atom)
{
    if (nerrors()) return;
    
    if (_functions.back().addLocal(atom) < 0) {
        m8r::String s = "param '";
        s += _program->stringFromAtom(atom);
        s += "' already exists";
        printError(ROMSTR("%s"), s.c_str());
    }
}

void Parser::functionStart(bool ctor)
{
    if (nerrors()) return;
    
    Mad<Function> func = Object::create<Function>();
    _functions.emplace_back(func, ctor);
}

void Parser::functionParamsEnd()
{
    if (nerrors()) return;
    
    _functions.back().markParamEnd();
}

Mad<Function> Parser::functionEnd()
{
    if (nerrors()) {
        return Mad<Function>();
    }
    
    assert(_functions.size());
    
    // If this is a ctor, we need to return this, just in case
    if (_functions.back()._ctor) {
        pushThis();
        emitCallRet(m8r::Op::RET, RegOrConst(), 1);
    }
    
    emitEnd();
    uint8_t tempRegisterCount = MaxRegister + 1 - _functions.back()._minReg;

    reconcileRegisters(_functions.back()._locals.size());
        
    // Place the current code and constants in this function
    Mad<Function> function = currentFunction();
    function->setCode(currentCode());
    function->setConstants(currentConstants());
    function->setLocalCount(_functions.back()._locals.size() + tempRegisterCount);
    
    _functions.pop_back();

    return function;
}

static inline uint32_t regFromTempReg(uint32_t reg, uint32_t numLocals)
{
    return (reg > numLocals && reg <= MaxRegister) ? (MaxRegister - reg + numLocals) : reg;
}

void Parser::reconcileRegisters(uint16_t localCount)
{
    assert(currentCode().size());
    
    uint8_t* code = &(currentCode()[0]);
    const uint8_t* end = code + currentCode().size();

    for (uint8_t* p = code; ; ) {
        if (p >= end) {
            return;
        }

        Op op = static_cast<Op>(*p++ & 0x3f);
        
        if (OpInfo::aReg(op)) {
            assert(*p <= MaxRegister);
            *p = regFromTempReg(*p, localCount);
            p += constantSize(*p) + 1;
        }
        if (OpInfo::bReg(op)) {
            *p = regFromTempReg(*p, localCount);
            p += constantSize(*p) + 1;
        }
        if (OpInfo::cReg(op)) {
            *p = regFromTempReg(*p, localCount);
            p += constantSize(*p) + 1;
        }
        if (OpInfo::dReg(op)) {
            assert(*p <= MaxRegister);
            *p = regFromTempReg(*p, localCount);
            p += constantSize(*p) + 1;
        }
        if (OpInfo::params(op)) {
            p++;
        }
        if (OpInfo::number(op)) {
            p += 2;
        }
    }
}

void Parser::ParseStack::push(ParseStack::Type type, RegOrConst reg)
{
    assert(type != Type::Register);
    _stack.push({ type, reg });
}

Parser::RegOrConst Parser::ParseStack::pushRegister()
{
    FunctionEntry& entry = _parser->_functions.back();
    uint32_t reg = entry._nextReg--;
    if (reg < entry._minReg) {
        entry._minReg = reg;
    }
    _stack.push({ Type::Register, RegOrConst(reg) });
    return RegOrConst(reg);
}

void Parser::ParseStack::pop()
{
    if (empty()) {
        return;
    }
    if (_stack.top()._type == Type::Register) {
        assert(_parser->_functions.back()._nextReg < MaxRegister);
        _parser->_functions.back()._nextReg++;
    }
    _stack.pop();
}

void Parser::ParseStack::swap()
{
    assert(_stack.size() >= 2);
    Entry t = _stack.top();
    _stack.top() = _stack.top(-1);
    _stack.top(-1) = t;
}

Parser::RegOrConst Parser::ParseStack::bake(bool makeClosure)
{
    Entry entry = _stack.top();
    
    switch(entry._type) {
        case Type::PropRef:
        case Type::EltRef: {
            if (entry._isValue) {
                // Currently TOS is a PropRef and TOS-1 is the source. We need to convert the
                // PropRef into a simple register, then swap it back to its original position,
                // then push a "setValue" atom, the swap again. Then we will have:
                // TOS=>src, Atom(setValue), dst. Now we need to generate the equivalent of:
                //
                //      dst.setValue(src
                //
                propRefToReg();
                _parser->emitId(Atom(SA::getValue), Parser::IdType::NotLocal);
                RegOrConst objectReg = _parser->emitDeref(Parser::DerefType::Prop);
                _parser->emitCallRet(Op::CALL, objectReg, 0);
                return _stack.top()._reg;
            } else {
                pop();
                RegOrConst r = pushRegister();
                _parser->emitCode((entry._type == Type::PropRef) ? Op::LOADPROP : Op::LOADELT, r, entry._reg, entry._derefReg);
                return r;
            }
        }
        case Type::RefK: {
            pop();
            RegOrConst r = pushRegister();
            if (entry._isValue) {
                pushRegister();
                _parser->emitId(Atom(SA::getValue), Parser::IdType::MightBeLocal);
                _parser->emitMove();
                _parser->emitCallRet(Op::CALL, RegOrConst(), 0);
                _parser->emitMove();
            } else {
                _parser->emitCode(Op::LOADREFK, r, entry._reg);
            }
            return r;
        }
        case Type::This: {
            pop();
            RegOrConst r = pushRegister();
            _parser->emitCode(Op::LOADTHIS, r);
            return r;
        }
        case Type::UpValue: {
            pop();
            RegOrConst r = pushRegister();
            _parser->emitCode(Op::LOADUP, r, entry._reg);
            return r;
        }
        case Type::Constant: {
            RegOrConst r = entry._reg;
            if (makeClosure) {
                assert(!r.isReg());
                int32_t index = r.index() - MaxRegister - 1 - builtinConstantOffset();
                Value v;
                
                if (index >= 0) {
                    v = _parser->currentConstants().at(index);
                }

                Mad<Object> func = v.asObject();
                if (func.valid() && func->canMakeClosure()) {
                    pop();
                    RegOrConst dst = pushRegister();
                    _parser->emitCode(Op::CLOSURE, dst, r);
                    r = dst;
                }
            }
            return r;
        }
        case Type::Local:
        case Type::Register:
            return entry._reg;
        case Type::Unknown:
        default:
            assert(0);
            return RegOrConst();
    }
}

void Parser::ParseStack::replaceTop(Type type, RegOrConst reg, RegOrConst derefReg)
{
    _stack.setTop({ type, reg, derefReg });
}

void Parser::ParseStack::propRefToReg()
{
    assert(_stack.top()._type == Type::PropRef);
    RegOrConst r = _stack.top()._reg;
    Type type = r.isReg() ? ((r.index() < _parser->_functions.back()._minReg) ? Type::Local : Type::Register) : Type::Register;
    replaceTop(type, r, RegOrConst());
}

#endif
