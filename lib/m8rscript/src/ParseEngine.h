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

namespace m8rscript {

class Function;
class Value;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ParseEngine
//
//////////////////////////////////////////////////////////////////////////////

/*

BNF:

program:
    { statement }

statement:
      functionStatement
    | classStatement
    | compoundStatement
    | selectionStatement
    | switchStatement
    | iterationStatement
    | jumpStatement
    | varStatement
    | expressionStatement
    ;
  
functionStatement:
    'function' identifier functionExpression ;

classStatement:
    'class' identifier classExpression ;

compoundStatement:
    '{' { statement } '}' ;

selectionStatement:
    'if' '(' commaExpression ')' statement 'else' statement ;

switchStatement:
    'switch' '(' commaExpression ')' '{' { caseClause } '}' ;
    
iterationStatement:
      'while' '(' commaExpression ')' statement
    | 'do' statement 'while' '(' commaExpression ')' ';'
    | 'for' '(' 'var' identifier ':' commaExpression ')' statement
    | 'for' '(' 'var' variableDeclarationList ';' commaExpression ';' commaExpression ')' statement
    | 'for' '(' commaExpression ':' commaExpression ')' statement
    | 'for' '(' commaExpression ';' commaExpression ';' commaExpression ')' statement
    ;
    
jumpStatement:
      'break' ';'
    | 'continue' ';'
    | 'return' commaExpression ';'
    ;
    
varStatement:
    'var' variableDeclarationList ';' ;
    
expressionStatement:
    commaExpression ';' ;
    
functionExpression:
    '(' formalParameterList ')' '{' { statement } '}' ;

classExpression:
    '{' { classContents } '}' ;

primaryExpression:
      '(' commaExpression ')'
    | identifier
    | 'this'
    | float
    | integer
    | string
    | boolean
    | 'null'
    | 'undefined'
    | '[' ']'
    | '[' arithmeticExpression { ',' arithmeticExpression } ']'
    | '{' '}'
    | '{' propertyAssignment { ',' propertyAssignment }
    ;

postfixExpression:
      primaryExpression
    | primaryExpression '++'
    | primaryExpression '--'
    | primaryExpression '(' argumentList ')'
    | primaryExpression '[' commaExpression ']'
    | primaryExpression '.' identifier
    ;

objectExpression:
      'new' primaryExpression { '(' argumentList ')'
    | 'delete' unaryExpression
    | 'function' functionExpression
    | 'class' classExpression
    ;

unaryExpression:
      objectExpression
    | postfixExpression
    | '++' unaryExpression
    | '--' unaryExpression
    | '-' unaryExpression
    | '~' unaryExpression
    | '!' unaryExpression
    ;
    
arithmeticExpression:
      unaryExpression
    | unaryExpression '?' commaExpression ':' arithmeticExpression
    | unaryExpression operator arithmeticExpression

commaExpression:
    arithmeticExpression { ',' arithmeticExpression } ;

formalParameterList:
      (* empty *)
    | identifier { ',' identifier }
      ;

propertyName:
      identifier
    | string
    | float
    | integer
    ;
      
propertyAssignment:
    propertyName ':' arithmeticExpression ;

variableDeclaration:
    identifier { '=' arithmeticExpression } ;

variableDeclarationList:
      (* empty *)
    | variableDeclaration { ',' variableDeclaration }
    ;

argumentList:
        (* empty *)
      | arithmeticExpression { ',' arithmeticExpression }
      ;

classContents:
      'function' identifier functionExpression
    | 'constructor' functionExpression
    | 'var' identifier { '=' (float | integer | string | boolean | 'null' | 'undefined')
    ;
    
operator: (* operator   precedence   association *)
               '='     (*   1          Right    *)
    |          '+='    (*   1          Right    *)
    |          '-='    (*   1          Right    *)
    |          '*='    (*   1          Right    *)
    |          '/='    (*   1          Right    *)
    |          '%='    (*   1          Right    *)
    |          '<<='   (*   1          Right    *)
    |          '>>='   (*   1          Right    *)
    |          '>>>='  (*   1          Right    *)
    |          '&='    (*   1          Right    *)
    |          '|='    (*   1          Right    *)
    |          '^='    (*   1          Right    *)
    |          '||'    (*   2          Left     *)
    |          '&&'    (*   3          Left     *)
    |          '|'     (*   4          Left     *)
    |          '^'     (*   5          Left     *)
    |          '&'     (*   6          Left     *)
    |          '=='    (*   7          Left     *)
    |          '!='    (*   7          Left     *)
    |          '<'     (*   8          Left     *)
    |          '>'     (*   8          Left     *)
    |          '>='    (*   8          Left     *)
    |          '<='    (*   8          Left     *)
    |          '<<'    (*   9          Left     *)
    |          '>>'    (*   9          Left     *)
    |          '<<<'   (*   9          Left     *)
    |          '+'     (*   10         Left     *)
    |          '-'     (*   10         Left     *)
    |          '*'     (*   11         Left     *)
    |          '/'     (*   11         Left     *)
    |          '%'     (*   11         Left     *)
    ;
    
identifier: idFirst { idFirst | digit } ;
 
float:
      { digit } '.' { digit }
    | { digit } ('e' | 'E') { '-' } dight { digit }
    | { digit } '.' { digit } ('e' | 'E') { '-' } dight { digit }
    ;
    
integer: digit { digit } ;
 
string:
      '\'' (* any character except '\''*) '\''
    | '"' (* any character except '"'*) '"'
 
boolean: 'true' | 'false' ;
 
idFirst: letter | '$' '_' ;
 
letter:
      'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G'
    | 'H' | 'I' | 'J' | 'K' | 'L' | 'M' | 'N'
    | 'O' | 'P' | 'Q' | 'R' | 'S' | 'T' | 'U'
    | 'V' | 'W' | 'X' | 'Y' | 'Z' | 'a' | 'b'
    | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i'
    | 'j' | 'k' | 'l' | 'm' | 'n' | 'o' | 'p'
    | 'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w'
    | 'x' | 'y' | 'z'
    ;
    
digit: '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' ;

*/

class ParseEngine  {
public:
    enum class Keyword {
        Unknown     = 0x00,
        Break       = 0x01,
        Case        = 0x02,
        Class       = 0x03,
        Constructor = 0x04,
        Continue    = 0x05,
        Default     = 0x06,
        Delete      = 0x07,
        Do          = 0x08,
        Else        = 0x09,
        For         = 0x0a,
        Function    = 0x0b,
        If          = 0x0c,
        New         = 0x0d,
        Return      = 0x0e,
        Switch      = 0x0f,
        This        = 0x10,
        Var         = 0x11,
        While       = 0x12,

        ADDSTO      = 0x13,
        SUBSTO      = 0x14,
        MULSTO      = 0x15,
        DIVSTO      = 0x16,
        MODSTO      = 0x17,
        SHLSTO      = 0x18,
        SHRSTO      = 0x19,
        SARSTO      = 0x1a,
        ANDSTO      = 0x1b,
        ORSTO       = 0x1c,
        XORSTO      = 0x1d,
        LOR         = 0x1e,
        LAND        = 0x1f,
        
        Bang        = '!',
        Percent     = '%',
        Ampersand   = '&',
        LParen      = '(',
        RParen      = ')',
        Star        = '*',
        Plus        = '+',
        Comma       = ',',
        Minus       = '-',
        Period      = '.',
        Slash       = '/',
        Colon       = ':',
        Semicolon   = ';',
        LT          = '<',
        STO         = '=',
        GT          = '>',
        Question    = '?',
        LBracket    = '[',
        RBracket    = ']',
        XOR         = '^',
        LBrace      = '{',
        OR          = '|',
        RBrace      = '}',
        Twiddle     = '~',
        At          = '@',
        Dollar      = '$',

        EQ          = 0x40,       
        NE          = 0x41,       
        GE          = 0x42,       
        LE          = 0x43,       
        SHL         = 0x44,       
        SHR         = 0x45,       
        SAR         = 0x46,       
        INCR        = 0x47,       
        DECR        = 0x48,       
    };

  	ParseEngine(Parser* parser);
  	
  	~ParseEngine()
  	{
    }
    
    // This assumes an enclosing function is in the function stack.
    // All the top level statements are placed in that function
    void program();
  
private:
    class OperatorInfo {
    public:
        enum class Assoc { Left = 0, Right = 1 };
        
        OperatorInfo() { }
        OperatorInfo(Keyword keyword, uint8_t prec, Assoc assoc, bool sto, Op op)
        {
            OperatorInfo info;
            info._token = static_cast<uint32_t>(keyword);
            info._prec = prec;
            info._op = static_cast<uint32_t>(op);
            info._assoc = static_cast<uint32_t>(assoc);
            info._sto = sto;
            _u = info._u;
        }
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
    bool expect(Parser::Expect expect, bool expected = false, const char* = nullptr);
    
    Keyword scanKeyword();
    Keyword tokenToKeyword();

    Token getToken() { return _parser->getToken(); }
    const Scanner::TokenType& getTokenValue() { return _parser->getTokenValue(); }
    void retireToken() { _parser->retireToken(); }

    bool statement();

    bool functionStatement();
    bool classStatement();
    bool compoundStatement();
    bool selectionStatement();
    bool switchStatement();
    bool iterationStatement();
    bool jumpStatement();
    bool varStatement();
    bool expressionStatement();
    
    using CaseEntry = struct { Label toStatement; Label fromStatement; int32_t statementAddr; };

    bool caseClause(Vector<CaseEntry>& cases, int32_t &defaultStatement, 
                    Label& defaultFromStatementLabel, bool& haveDefault);

    bool classContents();
    uint32_t variableDeclarationList();
    bool variableDeclaration();
    
    uint32_t argumentList();
    void forLoopCondAndIt();
    void forIteration(Atom iteratorName);
    bool propertyAssignment();
    bool propertyName();
    void formalParameterList();
    

    bool primaryExpression();
    Mad<Function> functionExpression(bool ctor);
    bool classExpression();
    bool objectExpression();
    bool postfixExpression();
    bool unaryExpression();
    bool arithmeticExpression(uint8_t minPrec = 1);
    bool commaExpression();

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
