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

using namespace m8r;

extern int yyparse(Parser*);

uint32_t Parser::_nextLabelId = 1;

Parser::Parser(Stream* istream)
    : _scanner(this, istream)
    , _program(new Program)
{
    _currentFunction = _program->main();
	yyparse(this);
}

void Parser::printError(const char* s)
{
    ++_nerrors;
	printf("Error: %s on line %d\n", s, _scanner.lineno());
}

Label Parser::label()
{
    Label label;
    label.label = _currentFunction->codeSize();
    label.uniqueID = _nextLabelId++;
    return label;
}

void Parser::emit(StringId s)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHSX));
    _currentFunction->addCodeInt(s.rawStringId(), 4);
}

void Parser::emit(uint32_t value)
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

void Parser::emit(float value)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHF));
    _currentFunction->addCodeInt(*(reinterpret_cast<uint32_t*>(&value)), 4);
}

void Parser::emit(const Atom& atom)
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

void Parser::emit(Op value)
{
    _currentFunction->addCode(static_cast<uint8_t>(value));
}

void Parser::emit(Object* obj)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHO));
    _currentFunction->addCodeInt(_program->addObject(obj).rawObjectId(), 4);
}

void Parser::addNamedFunction(Function* function, const Atom& name)
{
    function->setName(name);
    assert(name.valid());
    if (!name.valid()) {
        return;
    }
    _currentFunction->setValue(name, Value(function));
}

void Parser::addMatchedJump(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JF);
    label.matchedAddr = static_cast<int32_t>(_currentFunction->codeSize());
    _currentFunction->addCode(static_cast<uint8_t>(op) | 1);
    _currentFunction->addCodeInt(0, 2);
}

void Parser::matchJump(Label& label)
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
        _currentFunction->addCodeInt(jumpAddr, 2);
    }
    
    jumpAddr = static_cast<int32_t>(_currentFunction->codeSize()) - label.matchedAddr - 1;
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printf("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n");
        return;
    }
    _currentFunction->setCodeAtIndex(label.matchedAddr + 1, Function::byteFromInt(jumpAddr, 1));
    _currentFunction->setCodeAtIndex(label.matchedAddr + 2, Function::byteFromInt(jumpAddr, 0));
}

void Parser::emitWithCount(Op value, uint32_t nparams)
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

void Parser::emitDeferred()
{
    assert(!_deferred);
    assert(_deferredCode.size() > 0);
    for (auto c : _deferredCode.back()) {
        _currentFunction->addCode(c);
    }
    _deferredCode.pop_back();
}

void Parser::functionAddParam(const Atom& atom)
{
    if (!_currentFunction->addLocal(atom)) {
        String s = "param '";
        s += _program->stringFromAtom(atom);
        s += "' already exists";
        printError(s.c_str());
    }
}

void Parser::functionStart()
{
    _functions.push_back(_currentFunction);
    _currentFunction = new Function();
}

void Parser::functionParamsEnd()
{
    _currentFunction->markParamEnd();
}

Function* Parser::functionEnd()
{
    assert(_currentFunction && _functions.size());
    Function* function = _currentFunction;
    _currentFunction = _functions.back();
    _functions.pop_back();
    return function;
}

void Parser::programEnd()
{
    emit(Op::END);
}
