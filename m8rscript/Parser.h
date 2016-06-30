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

#pragma once

#include "Stream.h"
#include "Scanner.h"
#include "ExecutionUnit.h"
#include "Program.h"
#include "Array.h"

namespace m8r {

class Printer;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Parser
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Parser  {
public:
	Parser(m8r::Stream* istream, Printer* printer = nullptr);
    
    ~Parser()
    {
    }

  	Token getToken(Scanner::TokenType& token) { return _scanner.getToken(token); }
    
	void printError(const char* s);
    uint32_t nerrors() const { return _nerrors; }
    Program* program() { return _program; }
    
    m8r::String stringFromAtom(const Atom& atom) const { return _program->stringFromAtom(atom); }
    Atom atomizeString(const char* s) { return _program->atomizeString(s); }

    StringLiteral startString() { return _program->startString(); }
    void addToString(char c) { _program->addToString(c); }
    void endString() { _program->endString(); }
    
    // The next 3 functions work together:
    //
    // Label has a current location which is filled in by the label() call,
    // and a match location which is filled in by the addMatchedJump() function.
    // addMatchedJump also adds the passed Op (which can be JMP, JT or JF)
    // with an empty jump address, to be filled in my matchJump().
    // 
    // When matchJump() is called it adds a JMP to the current location in
    // the Label and then fixed up the match location with the location just
    // past the JMP
    //
    Label label();
    void addMatchedJump(Op op, Label&);
    void matchJump(Label&);
    void jumpToLabel(Op op, Label&);
    
    void startDeferred()
    {
        assert(!_deferred);
        _deferred = true;
        _deferredCodeBlocks.push_back(_deferredCode.size());
    }
    
    void endDeferred() { assert(_deferred); _deferred = false; }
    void emitDeferred();
    
    void functionAddParam(const Atom& atom);
    void functionStart();
    void functionParamsEnd();
    Function* functionEnd();
    void programEnd();
        
    void emit(StringLiteral::Raw value);
    void emit(uint64_t value);
    void emit(Float value);
    void emit(Op value);
    
    enum class IdType : uint8_t { MustBeLocal, MightBeLocal, NotLocal };
    void emitId(const Atom& value, IdType);
    
    void emit(Object* obj);
    void addNamedFunction(Function* value, const Atom& name);
    void emitWithCount(Op value, uint32_t count);
    void addVar(const Atom& name) { _currentFunction->addLocal(name); }
    
private:
    static uint8_t byteFromInt(uint64_t value, uint32_t index)
    {
        assert(index < 8);
        return static_cast<uint8_t>(value >> (8 * index));
    }
    
    void addCodeInt(uint64_t value, uint32_t size)
    {
        assert(size <= 8);
        for (int i = size - 1; i >= 0; --i) {
            addCodeByte(byteFromInt(value, i));
        }
    }
    
    void addCodeByte(Op op) { addCodeByte(static_cast<uint8_t>(op)); }
    void addCodeByte(Op op, uint32_t mask) { addCodeByte(static_cast<uint8_t>(op) | mask); }
    void addCodeByte(uint8_t);
    
    Scanner _scanner;
    Program* _program;
    Function* _currentFunction;
    Vector<Function*> _functions;
    uint32_t _nerrors = 0;
    Vector<size_t> _deferredCodeBlocks;
    Code _deferredCode;
    bool _deferred = false;
    
    Printer* _printer;

    static uint32_t _nextLabelId;
    
};

}
