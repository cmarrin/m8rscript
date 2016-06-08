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

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Parser
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Parser  {
public:
	Parser(Stream* istream);
    
    ~Parser()
    {
        delete _currentFunction;
        for (uint32_t i = 0; i < _functions.size(); ++i) {
            delete _functions[i];
        }
    }
    
    String toString() const { return _eu.stringFromCode(0, _program->main()); }
    
  	uint8_t getToken(YYSTYPE* token) { return _scanner.getToken(token); }
    
	void printError(const char* s);
    uint32_t nerrors() const { return _scanner.nerrors(); }
    
    void stringFromAtom(String& s, const Atom& atom) const { _program->stringFromAtom(s, atom); }
    void stringFromRawAtom(String& s, uint16_t rawAtom) const { _program->stringFromRawAtom(s, rawAtom); }
    Atom atomizeString(const char* s) { return _program->atomizeString(s); }
    
    Label label();
    void loopStart(bool cond, Label&);
    void loopEnd(Label&);
    
    void functionAddParam(const Atom& atom) { _currentFunction->addParam(atom); }
    void functionStart();
    Function* functionEnd();
    void programEnd();
        
    void emit(const char* value) { _eu.addCode(value); }
    void emit(uint32_t value) { _eu.addCode(value); }
    void emit(float value) { _eu.addCode(value); }
    void emit(const Atom& value) { _eu.addCode(value); }
    void emit(Op value) { _eu.addCode(value); }
    void addObject(Object* value) { _eu.addObject(value); }
    void addNamedFunction(Function* value, const Atom& name) { _eu.addNamedFunction(value, name); }
    void setFunction(Function* value) { _eu.setFunction(value); }
    void emitCallOrNew(bool call, uint32_t nparams) { _eu.addCallOrNew(call, nparams); }

private:
    Scanner _scanner;
    Program* _program;
    ExecutionUnit _eu;
    Function* _currentFunction;
    Vector<Function*> _functions;
};

}
