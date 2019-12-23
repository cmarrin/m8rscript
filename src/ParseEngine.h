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
    class OperatorInfo {
    public:
        enum class Assoc { Left = 0, Right = 1 };
        
        OperatorInfo() { }
        OperatorInfo(Token token, uint8_t prec, Assoc assoc, bool sto, Op op)
        {
            OperatorInfo info;
            info._token = static_cast<uint32_t>(token);
            info._prec = prec;
            info._op = static_cast<uint32_t>(op);
            info._assoc = static_cast<uint32_t>(assoc);
            info._sto = sto;
            _u = info._u;
        }
        
        bool operator==(const Token& t)
        {
            return static_cast<Token>(_token) == t;
        }
        
        Token token() const { return static_cast<Token>(_token); }
        uint8_t prec() const { return _prec; }
        Op op() const { return static_cast<Op>(_op); }
        bool sto() const { return _sto != 0; }
        Assoc assoc() const { return static_cast<Assoc>(_assoc); }

    private:
        union {
            struct {
                uint32_t _token : 8;
                uint32_t _prec : 8;
                uint32_t _op : 8;
                uint32_t _assoc : 1;
                uint32_t _sto : 1;
            };
            uint32_t _u;
        };
    };
    
    // OperatorInfo is in Flash, so we need to access it as a single 4 byte read
    static_assert(sizeof(OperatorInfo) == 4, "OperatorInfo must fit in 4 bytes");

    bool expect(Token token);
    bool expect(Token token, bool expected, const char* = nullptr);
    
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
    
    Mad<Function> functionExpression(bool ctor);
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
    
    Vector<Vector<Label>> _breakStack;
    Vector<Vector<Label>> _continueStack;

    struct CompareTokens
    {
        int operator()(const Token& lhs, const Token& rhs) const { return static_cast<int>(lhs) - static_cast<int>(rhs); }
    };

    static OperatorInfo _opInfos[ ];

};

}
