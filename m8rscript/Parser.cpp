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
    : _scanner(this)
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
    currentFunction()->setCodeAtIndex(label.matchedAddr + 1, ExecutionUnit::byteFromInt(jumpAddr, 1));
    currentFunction()->setCodeAtIndex(label.matchedAddr + 2, ExecutionUnit::byteFromInt(jumpAddr, 0));
}

void Parser::jumpToLabel(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JT);
    int32_t jumpAddr = label.label - static_cast<int32_t>(currentFunction()->code()->size()) - 1;
    
    uint32_t r = 0;
    if (op != Op::JMP) {
        bake();
        ParseStackEntry value = _parseStack.top();
        popParseStackEntry();
        r = value._reg;
    }

    emitCodeRSN(op, (op == Op::JMP) ? 0 : r, jumpAddr);
}

void Parser::emitCodeRRR(Op op, uint32_t ra, uint32_t rb, uint32_t rc)
{
    addCode((static_cast<uint32_t>(op) << 26) | ((ra & 0xff) << 18) | ((rb & 0x1ff) << 9) | (rc & 0x1ff));
}

void Parser::emitCodeRUN(Op op, uint32_t ra, uint32_t n)
{
    addCode((static_cast<uint32_t>(op) << 26) | ((ra & 0xff) << 18) | (n & 0x3ffff));
}

void Parser::emitCodeRSN(Op op, uint32_t ra, int32_t n)
{
    addCode((static_cast<uint32_t>(op) << 26) | ((ra & 0xff) << 18) | (n & 0x3ffff));
}

void Parser::addCode(uint32_t c)
{
    if (_deferred) {
        assert(_deferredCodeBlocks.size() > 0);
        _deferredCode.push_back(c);
    } else {
        currentFunction()->addCode(c);
    }
}

void Parser::pushK(StringLiteral::Raw s)
{
    ConstantId id = currentFunction()->addConstant(StringLiteral(s));
    _parseStack.push({ ParseStackEntry::Type::Constant, id.raw() });
}

void Parser::pushK(uint32_t value)
{
    ConstantId id = currentFunction()->addConstant(value);
    _parseStack.push({ ParseStackEntry::Type::Constant, id.raw() });
}

void Parser::pushK(Float value)
{
    ConstantId id = currentFunction()->addConstant(value);
    _parseStack.push({ ParseStackEntry::Type::Constant, id.raw() });
}

void Parser::pushK(bool value)
{
    // FIXME: Support booleans as a first class type
    ConstantId id = currentFunction()->addConstant(value ? 1 : 0);
    _parseStack.push({ ParseStackEntry::Type::Constant, id.raw() });
}

void Parser::pushK()
{
    // FIXME: Represent Null as its own value type to distinguish it from and error
    ConstantId id = currentFunction()->addConstant(Value());
    _parseStack.push({ ParseStackEntry::Type::Constant, id.raw() });
}

void Parser::pushK(ObjectId function)
{
    ConstantId id = currentFunction()->addConstant(function);
    _parseStack.push({ ParseStackEntry::Type::Constant, id.raw() });
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
            _parseStack.push({ ParseStackEntry::Type::Local, static_cast<uint32_t>(index) });
            return;
        }
    }
    
    ConstantId id = currentFunction()->addConstant(atom);
    _parseStack.push({ (type == IdType::NotLocal) ? ParseStackEntry::Type::Constant : ParseStackEntry::Type::RefK, id.raw() });
}

void Parser::bake()
{
    ParseStackEntry entry = _parseStack.top();
    
    if (entry._type == ParseStackEntry::Type::PropRef || entry._type == ParseStackEntry::Type::EltRef) {
        popParseStackEntry();
        uint32_t r = pushParseStackEntry(ParseStackEntry::Type::Register);
        emitCodeRRR((entry._type == ParseStackEntry::Type::PropRef) ? Op::LOADPROP : Op::LOADELT, r, entry._reg, entry._derefReg);
    }
}

void Parser::emitMove()
{
    bake();
    ParseStackEntry srcValue = _parseStack.top();
    popParseStackEntry();
    ParseStackEntry dstValue = _parseStack.top();
    
    switch(dstValue._type) {
        case ParseStackEntry::Type::PropRef:
        case ParseStackEntry::Type::EltRef: {
            emitCodeRRR((dstValue._type == ParseStackEntry::Type::PropRef) ? Op::STOPROP : Op::STOELT, srcValue._reg, dstValue._reg, dstValue._derefReg);
            break;
        default:
            emitCodeRRR(Op::MOVE, dstValue._reg, srcValue._reg);
            break;
        }
    }
}

void Parser::emitDeref(bool prop)
{
    bake();
    ParseStackEntry derefValue = _parseStack.top();
    popParseStackEntry();
    bake();
    ParseStackEntry objectValue = _parseStack.top();
    popParseStackEntry();
    pushParseStackEntry(prop ? ParseStackEntry::Type::PropRef : ParseStackEntry::Type::EltRef, objectValue._reg, derefValue._reg);
}

void Parser::emitDup()
{
    ParseStackEntry entry = _parseStack.top();
    switch(entry._type) {
        case ParseStackEntry::Type::PropRef:
        case ParseStackEntry::Type::EltRef: {
            uint32_t r = pushParseStackEntry(ParseStackEntry::Type::Register);
            emitCodeRRR((entry._type == ParseStackEntry::Type::PropRef) ? Op::LOADPROP : Op::LOADELT, r, entry._reg, entry._derefReg);
            break;
        }
        
        case ParseStackEntry::Type::Local:
            pushParseStackEntry(ParseStackEntry::Type::Local, entry._reg);
            break;
        default:
            assert(0);
            break;
    }
}

void Parser::emitStoProp()
{
    // tos-2 object to store into
    // tos-1 property of this object to store into
    // tos value to store
    bake();
    ParseStackEntry value = _parseStack.top();
    popParseStackEntry();
    bake();
    ParseStackEntry derefValue = _parseStack.top();
    popParseStackEntry();
    bake();
    ParseStackEntry  objectValue = _parseStack.top();
    
    emitCodeRRR(Op::STOPROP, objectValue._reg, derefValue._reg, value._reg);
}

void Parser::emitBinOp(Op op)
{
    if (op == Op::MOVE) {
        emitMove();
        return;
    }
    
    bake();
    ParseStackEntry leftValue = _parseStack.top();
    popParseStackEntry();
    bake();
    ParseStackEntry rightValue = _parseStack.top();
    popParseStackEntry();
    uint32_t dst = pushParseStackEntry(ParseStackEntry::Type::Register);

    emitCodeRRR(op, dst, leftValue._reg, rightValue._reg);
}

void Parser::emitUnOp(Op op)
{
    bake();
    ParseStackEntry srcValue = _parseStack.top();
    popParseStackEntry();
    uint32_t dst = pushParseStackEntry(ParseStackEntry::Type::Register);
    emitCodeRRR(op, dst, srcValue._reg);
}

void Parser::emitLoadLit(bool array)
{
    uint32_t dst = pushParseStackEntry(ParseStackEntry::Type::Register);
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
        ParseStackEntry callee = _parseStack.top(-nparams);
        pushParseStackEntry(callee._type, callee._reg, callee._derefReg);
        bake();
        callee = _parseStack.top();
        popParseStackEntry();
        calleeReg = callee._reg;
    }

    emitCodeRUN(value, calleeReg, nparams);
}

void Parser::emitDeferred()
{
    assert(!_deferred);
    assert(_deferredCodeBlocks.size() > 0);
    for (size_t i = _deferredCodeBlocks.back(); i < _deferredCode.size(); ++i) {
        currentFunction()->addCode(_deferredCode[i]);
    }
    _deferredCode.resize(_deferredCodeBlocks.back());
    _deferredCodeBlocks.pop_back();
}

uint32_t Parser::pushParseStackEntry(ParseStackEntry::Type type, uint32_t reg, uint32_t derefReg)
{
    if (type == ParseStackEntry::Type::Register) {
        reg = _functions.back()._nextReg--;
    }
    _parseStack.push({ type, reg, derefReg });
    return reg;
}

void Parser::popParseStackEntry()
{
    assert(!_parseStack.empty());
    if (_parseStack.top()._type == ParseStackEntry::Type::Register) {
        _functions.back()._nextReg++;
    }
    _parseStack.pop();
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
    
    // FIXME: Remember max temp reg count and pass it
    function->reconcileRegisters(0);
    
    return function->objectId();
}

void Parser::programEnd()
{
    emitEnd();
}
