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

#include "Atom.h"
#include "Containers.h"
#include "Parser.h"
#include "Opcodes.h"

namespace m8r {

class Function;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ParseEngine
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class ParseEngine  {
public:
    enum class Error : uint8_t { Expected };
    
  	ParseEngine(Parser* parser);
  	
  	~ParseEngine()
  	{
    }
  
    void program();

private:
    struct OpInfo {
        enum class Assoc : uint8_t { Left, Right };
        uint8_t prec;
        Assoc assoc;
        Op op;
    };
        
    bool expect(uint8_t token);
    bool expect(Token token, bool expected);
    void syntaxError(Error, Token token);
    
    void popToken() { _token = _parser->getToken(_tokenValue); }

    bool sourceElements();
    bool sourceElement();
    bool statement();
    bool functionDeclaration();
    bool compoundStatement();
    bool selectionStatement();
    bool switchStatement();
    bool iterationStatement();
    bool jumpStatement();
    bool variableDeclarationList();
    bool variableDeclaration();
    
    bool arithmeticPrimary();
    bool expression(uint8_t minPrec = 1);
    
    bool leftHandSideExpression();
    bool primaryExpression();
    
    Function* function();
    uint32_t argumentList();
    void forLoopCondAndIt();
    bool propertyAssignment();
    bool propertyName();
    void formalParameterList();
    
    Parser* _parser;
    uint8_t _token;
    Scanner::TokenType _tokenValue;
    
    static Map<uint8_t, OpInfo> _opInfo;
};

}
