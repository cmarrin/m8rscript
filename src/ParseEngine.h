/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "Parser.h"

namespace m8r {

class Function;
class Value;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ParseEngine
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class ParseEngine  {
public:
  	ParseEngine(Parser* parser);
  	
  	~ParseEngine()
  	{
    }
  
    bool statement();
    bool classContentsStatement();
    bool expression(uint8_t minPrec = 1);

private:
    struct OpInfo {
        static const uint8_t LeftAssoc = 0;
        static const uint8_t RightAssoc = 1;
        uint8_t prec : 6;
        uint8_t assoc : 1;
        uint8_t sto : 1;
        Op op;
    };
        
    bool expect(Token token);
    bool expect(Token token, bool expected);
    
    Token getToken() { return _parser->getToken(); }
    const Scanner::TokenType& getTokenValue() { return _parser->getTokenValue(); }
    void retireToken() { _parser->retireToken(); }

    bool functionStatement();
    bool classStatement();
    bool compoundStatement();
    bool selectionStatement();
    bool switchStatement();
    bool iterationStatement();
    bool jumpStatement();
    
    uint32_t variableDeclarationList();
    bool variableDeclaration();
    
    bool arithmeticPrimary();
    
    bool leftHandSideExpression();
    bool memberExpression();
    bool primaryExpression();
    
    Function* functionExpression(bool ctor);
    bool classExpression();
    uint32_t argumentList();
    void forLoopCondAndIt();
    void forIteration(Atom iteratorName);
    bool propertyAssignment();
    bool propertyName();
    void formalParameterList();
    
    Parser* _parser;
    Token _currentToken = Token::None;
    Scanner::TokenType _currentTokenValue;
    
    std::vector<std::vector<Label>> _breakStack;
    std::vector<std::vector<Label>> _continueStack;

    struct CompareTokens
    {
        int operator()(const Token& lhs, const Token& rhs) const { return static_cast<int>(lhs) - static_cast<int>(rhs); }
    };

    static Map<Token, OpInfo, CompareTokens> _opInfo;
};

}
