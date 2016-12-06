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
#include "SystemInterface.h"
#include <limits>

using namespace m8r;

uint32_t Parser::_nextLabelId = 1;

Parser::Parser(SystemInterface* system)
    : _parseStack(this)
    , _scanner(this)
    , _program(new Program(system))
    , _system(system)
{
    _functions.emplace_back(_program);
}

void Parser::parse(m8r::Stream* istream)
{
    _scanner.setStream(istream);
    ParseEngine p(this);
    while(1) {
        if (!p.statement()) {
            break;
        }
    }
    programEnd();
}

void Parser::printError(const char* s)
{
    ++_nerrors;
    if (_system) {
        _system->printf(ROMSTR("Error: %s on line %d\n"), s, _scanner.lineno());
    }
}

Label Parser::label()
{
    Label label;
    label.label = static_cast<int32_t>(currentFunction()->code()->size());
    label.uniqueID = _nextLabelId++;
    return label;
}

void Parser::addMatchedJump(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JF);
    label.matchedAddr = static_cast<int32_t>(currentFunction()->code()->size());
    emitCodeRRR(op);
}

void Parser::matchJump(Label& label)
{
    int32_t jumpAddr = static_cast<int32_t>(currentFunction()->code()->size()) - label.matchedAddr - 1;
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printError("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n");
        return;
    }
    
    uint32_t machineCode = currentFunction()->code()->at(label.matchedAddr);
    Op op = machineCodeToOp(machineCode);
    uint32_t reg = machineCodeToRa(machineCode);
    emitCodeRSN(op, reg, jumpAddr);
}

void Parser::jumpToLabel(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    int32_t jumpAddr = label.label - static_cast<int32_t>(currentFunction()->code()->size()) - 1;
    
    uint32_t r = 0;
    if (op != Op::JMP) {
        r = _parseStack.bake();
        _parseStack.pop();
    }

    emitCodeRSN(op, (op == Op::JMP) ? 0 : r, jumpAddr);
}

void Parser::emitCodeRRR(Op op, uint32_t ra, uint32_t rb, uint32_t rc)
{
    addCode(genMachineCodeRRR(op, ra, rb, rc));
}

void Parser::emitCodeRUN(Op op, uint32_t ra, uint32_t n)
{
    addCode(genMachineCodeRUN(op, ra, n));
}

void Parser::emitCodeRSN(Op op, uint32_t ra, int32_t n)
{
    addCode(genMachineCodeRSN(op, ra, n));
}

void Parser::addCode(uint32_t c)
{
    if (_deferred) {
        assert(_deferredCodeBlocks.size() > 0);
        _deferredCode.push_back(c);
    } else {
        currentFunction()->code()->push_back(c);
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
            emitCodeRRR((dstType == ParseStack::Type::PropRef) ? Op::STOPROP : Op::STOELT, srcReg, _parseStack.topReg(), _parseStack.topDerefReg());
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
    _parseStack.pop();
    _parseStack.push(prop ? ParseStack::Type::PropRef : ParseStack::Type::EltRef, objectReg, derefReg);
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
    
    uint32_t leftReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t rightReg = _parseStack.bake();
    _parseStack.pop();
    uint32_t dst = _parseStack.push(ParseStack::Type::Register);

    emitCodeRRR(op, dst, leftReg, rightReg);
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

void Parser::emitEnd()
{
    emitCodeRRR(Op::END);
}

void Parser::emitWithCount(Op value, uint32_t nparams)
{
    assert(nparams < 256);
    assert(value == Op::CALL || value == Op::NEW || value == Op::RET);
    
    // If this is CALL or NEW, the callee is behind the params. We need to
    // Get it to the top of the parse stack so we can bake it. We leave
    // the original on the stack for the caller to pop on return
    uint32_t calleeReg = 0;
    
    if (value == Op::CALL || value == Op::NEW) {
        calleeReg = _parseStack.dupCallee(nparams);
    }

    emitCodeRUN(value, calleeReg, nparams);
}

void Parser::emitDeferred()
{
    assert(!_deferred);
    assert(_deferredCodeBlocks.size() > 0);
    for (size_t i = _deferredCodeBlocks.back(); i < _deferredCode.size(); ++i) {
        currentFunction()->code()->push_back(_deferredCode[i]);
    }
    _deferredCode.resize(_deferredCodeBlocks.back());
    _deferredCodeBlocks.pop_back();
}

void Parser::addNamedFunction(ObjectId functionId, const Atom& name)
{
    assert(name);
    int32_t index = currentFunction()->addProperty(name);
    assert(index >= 0);
    currentFunction()->setProperty(nullptr, index, functionId);
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
    ObjectId functionId = _program->addObject(currentFunction());
    currentFunction()->setObjectId(functionId);
}

void Parser::functionParamsEnd()
{
    currentFunction()->markParamEnd();
}

ObjectId Parser::functionEnd()
{
    assert(_functions.size());
    Function* function = currentFunction();
    _functions.pop_back();
    
    reconcileRegisters(function);
    
    return function->objectId();
}

void Parser::programEnd()
{
    emitEnd();
    reconcileRegisters(_program);
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
        uint32_t machineCode = code[i];
        Op op = machineCodeToOp(machineCode);
        uint32_t ra = regFromTempReg(machineCodeToRa(machineCode), numLocals);
        
        if (op == Op::RET || op == Op::CALL || op == Op::NEW) {
            code[i] = genMachineCodeRUN(op, ra, machineCodeToUN(machineCode));
        } else if (op == Op::JMP || op == Op::JT || op == Op::JF) {
            code[i] = genMachineCodeRSN(op, ra, machineCodeToSN(machineCode));
        } else {
            code[i] = genMachineCodeRRR(op, ra, regFromTempReg(machineCodeToRb(machineCode), numLocals), regFromTempReg(machineCodeToRc(machineCode), numLocals));
        }
    }
}

uint32_t Parser::ParseStack::push(ParseStack::Type type, uint32_t reg, uint32_t derefReg)
{
    if (type == Type::Register) {
        reg = _parser->_functions.back()._nextReg--;
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
    }
    return entry._reg;
}

uint32_t Parser::ParseStack::dupCallee(int32_t nparams)
{
    Entry callee = _stack.top(-nparams);
    _stack.push({ callee._type, callee._reg, callee._derefReg });
    bake();
    callee = _stack.top();
    _stack.pop();
    return callee._reg;
}


