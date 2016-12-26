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

#include "Parser.h"

#include "ParseEngine.h"
#include "ExecutionUnit.h"
#include <limits>

using namespace m8r;

uint32_t Parser::_nextLabelId = 1;

Parser::Parser()
    : _parseStack(this)
    , _scanner(this)
    , _program(new Program())
{
    _program->setCollectable(false);
}

void Parser::parse(m8r::Stream* istream)
{
    _scanner.setStream(istream);
    ParseEngine p(this);
    _functions.emplace_back(_program);
    while(1) {
        if (!p.statement()) {
            break;
        }
    }
    functionEnd();
}

void Parser::printError(const char* s)
{
    ++_nerrors;
    Error::printError(Error::Code::ParseError, _scanner.lineno(), s);
}

void Parser::unknownError(Token token)
{
    ++_nerrors;
    uint8_t c = static_cast<uint8_t>(token);
    Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("unknown token (%s)"), Value::toString(c).c_str());
}

void Parser::expectedError(Token token)
{
    ++_nerrors;
    uint8_t c = static_cast<uint8_t>(token);
    if (c < 0x80) {
        Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("syntax error: expected '%c'"), c);
    } else {
        switch(token) {
            case Token::DuplicateDefault: Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("multiple default cases not allowed")); break;
            case Token::Expr: Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("expression")); break;
            case Token::PropertyAssignment: Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("property assignment")); break;
            case Token::Statement: Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("statement expected")); break;
            case Token::Identifier: Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("identifier")); break;
            default: Error::printError(Error::Code::ParseError, _scanner.lineno(), ROMSTR("*** UNKNOWN TOKEN ***")); break;
        }
    }
}

Label Parser::label()
{
    Label label;
    label.label = static_cast<int32_t>(_deferred ? _deferredCode.size() : currentFunction()->code()->size());
    label.uniqueID = _nextLabelId++;
    return label;
}

void Parser::addMatchedJump(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JT || op == Op::JF);
    label.matchedAddr = static_cast<int32_t>(_deferred ? _deferredCode.size() : currentFunction()->code()->size());

    uint32_t reg = 0;
    if (op != Op::JMP) {
        reg = _parseStack.bake();
        _parseStack.pop();
    }
    // Emit opcode with a dummy address
    emitCodeRSN(op, reg, 0);
}

void Parser::doMatchJump(int32_t matchAddr, int32_t jumpAddr)
{
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printError(ROMSTR("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n"));
        return;
    }
    
    Instruction inst = currentFunction()->code()->at(matchAddr);
    Op op = static_cast<Op>(inst.op());
    uint32_t reg = inst.ra();
    currentFunction()->code()->at(matchAddr) = Instruction(op, reg, jumpAddr);
}

void Parser::jumpToLabel(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    int32_t jumpAddr = label.label - static_cast<int32_t>(currentFunction()->code()->size());
    
    uint32_t r = 0;
    if (op != Op::JMP) {
        r = _parseStack.bake();
        _parseStack.pop();
    }

    emitCodeRSN(op, (op == Op::JMP) ? 0 : r, jumpAddr);
}

void Parser::emitCodeRRR(Op op, uint32_t ra, uint32_t rb, uint32_t rc)
{
    addCode(Instruction(op, ra, rb, rc));
}

void Parser::emitCodeRUN(Op op, uint32_t rn, uint32_t n)
{
    addCode(Instruction(op, rn, n));
}

void Parser::emitCodeRSN(Op op, uint32_t rn, int32_t n)
{
    addCode(Instruction(op, rn, n));
}

void Parser::addCode(Instruction inst)
{
    if (_deferred) {
        assert(_deferredCodeBlocks.size() > 0);
        _deferredCode.push_back(inst);
    } else {
        currentFunction()->code()->push_back(inst);
    }
}

void Parser::pushK(StringLiteral::Raw s)
{
    ConstantId id = currentFunction()->addConstant(StringLiteral(s));
    _parseStack.push(ParseStack::Type::Constant, id.raw());
}

void Parser::pushK(uint32_t value)
{
    ConstantId id = currentFunction()->addConstant(value);
    _parseStack.push(ParseStack::Type::Constant, id.raw());
}

void Parser::pushK(Float value)
{
    ConstantId id = currentFunction()->addConstant(value);
    _parseStack.push(ParseStack::Type::Constant, id.raw());
}

void Parser::pushK(bool value)
{
    // FIXME: Support booleans as a first class type
    ConstantId id = currentFunction()->addConstant(value ? 1 : 0);
    _parseStack.push(ParseStack::Type::Constant, id.raw());
}

void Parser::pushK()
{
    // FIXME: Represent Null as its own value type to distinguish it from and error
    ConstantId id = currentFunction()->addConstant(Value());
    _parseStack.push(ParseStack::Type::Constant, id.raw());
}

void Parser::pushK(ObjectId function)
{
    Object* obj = _program->obj(function);
    assert(obj);
    obj->setCollectable(false);
    ConstantId id = currentFunction()->addConstant(function);
    _parseStack.push(ParseStack::Type::Constant, id.raw());
}

void Parser::emitId(const Atom& atom, IdType type)
{
    if (type == IdType::MightBeLocal || type == IdType::MustBeLocal) {
        int32_t index = currentFunction()->localIndex(atom);
        if (index < 0 && type == IdType::MustBeLocal) {
            String s = "nonexistent variable '";
            s += _program->stringFromAtom(atom);
            s += "'";
            printError(s.c_str());
        }
        if (index >= 0) {
            _parseStack.push(ParseStack::Type::Local, static_cast<uint32_t>(index));
            return;
        }
    }
    
    ConstantId id = currentFunction()->addConstant(atom);
    _parseStack.push((type == IdType::NotLocal) ? ParseStack::Type::Constant : ParseStack::Type::RefK, id.raw());
}

void Parser::emitMove()
{
    uint32_t srcReg = _parseStack.bake();
    _parseStack.pop();
    ParseStack::Type dstType = _parseStack.topType();
    
    switch(dstType) {
        case ParseStack::Type::PropRef:
        case ParseStack::Type::EltRef: {
            emitCodeRRR((dstType == ParseStack::Type::PropRef) ? Op::STOPROP : Op::STOELT, _parseStack.topReg(), _parseStack.topDerefReg(), srcReg);
            break;
        default:
            emitCodeRRR(Op::MOVE, _parseStack.topReg(), srcReg);
            break;
        }
    }
}

void Parser::emitDeref(bool prop)
{
    uint32_t derefReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t objectReg = _parseStack.bake();
    _parseStack.replaceTop(prop ? ParseStack::Type::PropRef : ParseStack::Type::EltRef, objectReg, derefReg);
}

void Parser::emitDup()
{
    ParseStack::Type type = _parseStack.topType();
    switch(type) {
        case ParseStack::Type::PropRef:
        case ParseStack::Type::EltRef: {
            uint32_t r = _parseStack.push(ParseStack::Type::Register);
            emitCodeRRR((type == ParseStack::Type::PropRef) ? Op::LOADPROP : Op::LOADELT, r, _parseStack.topReg(), _parseStack.topDerefReg());
            break;
        }
        
        case ParseStack::Type::Local:
            _parseStack.push(ParseStack::Type::Local, _parseStack.topReg());
            break;
        default:
            assert(0);
            break;
    }
}

void Parser::emitAppendElt()
{
    // tos-1 object to append to
    // tos value to store
    // leave object on tos
    uint32_t srcReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t objectReg = _parseStack.bake();
    
    emitCodeRRR(Op::APPENDELT, objectReg, srcReg);
}

void Parser::emitAppendProp()
{
    // tos-2 object to append to
    // tos-1 property on that object
    // tos value to store
    // leave object on tos
    uint32_t srcReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t propReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t objectReg = _parseStack.bake();
    
    emitCodeRRR(Op::APPENDPROP, objectReg, propReg, srcReg);
}

void Parser::emitStoProp()
{
    // tos-2 object to store into
    // tos-1 property of this object to store into
    // tos value to store
    // leave object on tos
    uint32_t srcReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t derefReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t objectReg = _parseStack.bake();
    
    emitCodeRRR(Op::STOPROP, objectReg, derefReg, srcReg);
}

void Parser::emitBinOp(Op op)
{
    if (op == Op::MOVE) {
        emitMove();
        return;
    }
    
    uint32_t rightReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t leftReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t dst = _parseStack.push(ParseStack::Type::Register);

    emitCodeRRR(op, dst, leftReg, rightReg);
}

void Parser::emitCaseTest()
{
    // This is like emitBinOp(Op::EQ), but does not pop the left operand
    uint32_t rightReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t leftReg = _parseStack.bake();
    uint32_t dst = _parseStack.push(ParseStack::Type::Register);

    emitCodeRRR(Op::EQ, dst, leftReg, rightReg);
}

void Parser::emitUnOp(Op op)
{
    uint32_t srcReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t dst = _parseStack.push(ParseStack::Type::Register);
    emitCodeRRR(op, dst, srcReg);
}

void Parser::emitLoadLit(bool array)
{
    uint32_t dst = _parseStack.push(ParseStack::Type::Register);
    emitCodeRRR(array ? Op::LOADLITA : Op::LOADLITO, dst);
}

void Parser::emitPush()
{
    uint32_t src = _parseStack.bake();
    _parseStack.pop();
    
    // Value pushed is a register or constant, so it has to go into rb
    emitCodeRUN(Op::PUSH, src, 0);
}

void Parser::emitPop()
{
    uint32_t dst = _parseStack.push(ParseStack::Type::Register);
    emitCodeRRR(Op::POP, dst);
}

void Parser::emitEnd()
{
    // If we have no errors we expect an empty stack, otherwise, there might be cruft left over
    if (_nerrors) {
        _parseStack.clear();
    }
    assert(_parseStack.empty());
    emitCodeRRR(Op::END);
}

void Parser::emitWithCount(Op value, uint32_t nparams)
{
    assert(nparams < 256);
    assert(value == Op::CALL || value == Op::NEW || value == Op::RET);
    
    uint32_t calleeReg = 0;
    
    if (value == Op::CALL || value == Op::NEW) {
        calleeReg = _parseStack.bake();
        _parseStack.pop();
    } else {
        // If there is a return value, push it onto the runtime stack
        for (uint32_t i = 0; i < nparams; ++i) {
            emitPush();
        }
    }
        
    emitCodeRUN(value, calleeReg, nparams);
    
    if (value == Op::CALL || value == Op::NEW) {
        // On return there will be a value on the runtime stack. Pop it into a register
        emitPop();
    }
}

int32_t Parser::emitDeferred()
{
    assert(!_deferred);
    assert(_deferredCodeBlocks.size() > 0);
    int32_t start = static_cast<int32_t>(currentFunction()->code()->size());
    
    for (size_t i = _deferredCodeBlocks.back(); i < _deferredCode.size(); ++i) {
        currentFunction()->code()->push_back(_deferredCode[i]);
    }
    _deferredCode.resize(_deferredCodeBlocks.back());
    _deferredCodeBlocks.pop_back();
    return start;
}

void Parser::addNamedFunction(ObjectId functionId, const Atom& name)
{
    assert(name);
    Value* value = currentFunction()->addProperty(name);
    assert(value);
    *value = Value(functionId);
}

void Parser::functionAddParam(const Atom& atom)
{
    if (currentFunction()->addLocal(atom) < 0) {
        m8r::String s = "param '";
        s += _program->stringFromAtom(atom);
        s += "' already exists";
        printError(s.c_str());
    }
}

void Parser::functionStart()
{
    _functions.emplace_back(new Function());
    ObjectId functionId = _program->addObject(currentFunction(), true);
    currentFunction()->setObjectId(functionId);
}

void Parser::functionParamsEnd()
{
    currentFunction()->markParamEnd();
}

ObjectId Parser::functionEnd()
{
    assert(_functions.size());
    emitEnd();
    Function* function = currentFunction();
    uint8_t tempRegisterCount = 256 - _functions.back()._minReg;
    _functions.pop_back();
    
    reconcileRegisters(function);
    function->setTempRegisterCount(tempRegisterCount);
    
    return function->objectId();
}

static inline uint32_t regFromTempReg(uint32_t reg, uint32_t numLocals)
{
    return (reg > numLocals && reg < 256) ? (255 - reg + numLocals) : reg;
}

void Parser::reconcileRegisters(Function* function)
{
    assert(function->code());
    Code& code = *function->code();
    uint32_t numLocals = static_cast<uint32_t>(function->localSize());
    
    for (int i = 0; i < code.size(); ++i) {
        Instruction inst = code[i];
        Op op = static_cast<Op>(inst.op());
        uint32_t rn = regFromTempReg(inst.rn(), numLocals);
        
        if (op == Op::RET || op == Op::CALL || op == Op::NEW) {
            code[i] = Instruction(op, rn, inst.un());
        } else if (op == Op::JMP || op == Op::JT || op == Op::JF) {
            code[i] = Instruction(op, rn, inst.sn());
        } else if (op == Op::PUSH) {
            code[i] = Instruction(op, rn, inst.sn());
        } else {
            uint32_t ra = regFromTempReg(inst.ra(), numLocals);
            code[i] = Instruction(op, ra, regFromTempReg(inst.rb(), numLocals), regFromTempReg(inst.rc(), numLocals));
        }
    }
}

uint32_t Parser::ParseStack::push(ParseStack::Type type, uint32_t reg, uint32_t derefReg)
{
    if (type == Type::Register) {
        FunctionEntry& entry = _parser->_functions.back();
        reg = entry._nextReg--;
        if (reg < entry._minReg) {
            entry._minReg = reg;
        }
    }
    _stack.push({ type, reg, derefReg });
    return reg;
}

void Parser::ParseStack::pop()
{
    assert(!_stack.empty());
    if (_stack.top()._type == Type::Register) {
        _parser->_functions.back()._nextReg++;
    }
    _stack.pop();
}

uint32_t Parser::ParseStack::bake()
{
    Entry entry = _stack.top();
    
    if (entry._type == Type::PropRef || entry._type == Type::EltRef) {
        pop();
        uint32_t r = push(Type::Register);
        _parser->emitCodeRRR((entry._type == Type::PropRef) ? Op::LOADPROP : Op::LOADELT, r, entry._reg, entry._derefReg);
        return r;
    } else if (entry._type == Type::RefK) {
        pop();
        uint32_t r = push(Type::Register);
        _parser->emitCodeRRR(Op::LOADREFK, r, entry._reg);
        return r;
    }
    return entry._reg;
}

void Parser::ParseStack::replaceTop(Type type, uint32_t reg, uint32_t derefReg)
{
    _stack.setTop({ type, reg, derefReg });
}
