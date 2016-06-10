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

Parser::Parser(Stream* istream)
    : _scanner(this, istream)
    , _program(new Program)
    , _eu(this, _program)
{
    _currentFunction = _program->main();
    _eu.setFunction(_program->main());
	yyparse(this);
}

void Parser::printError(const char* s)
{
	printf("%s on line %d\n", s, _scanner.lineno());
}

Label Parser::label()
{
    Label lbl = _eu.label();
    return lbl;
}

void Parser::loopStart(bool cond, Label& label)
{
    _eu.addFixupJump(cond, label);
}

void Parser::loopEnd(Label& label)
{
    _eu.addJumpAndFixup(label);
}

void Parser::functionAddParam(const Atom& atom)
{
    if (!_currentFunction->addParam(atom)) {
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
    _eu.setFunction(_currentFunction);
}

Function* Parser::functionEnd()
{
    assert(_currentFunction && _functions.size());
    Function* function = _currentFunction;
    _currentFunction = _functions.back();
    _functions.pop_back();
    _eu.setFunction(_currentFunction);
    return function;
}

void Parser::programEnd()
{
    emit(Op::END);
}
