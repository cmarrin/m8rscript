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
    // There is one assumption for these value. Special chars
    // like Bang and Percent are the same as their m8r::Token
    // counterparts. And those values are the same as the 
    // ASCII code for the corresponding character. Furthermore
    // all non special char m8r::Tokens have values greater than
    // 0x7f. So you can test if a token is a special char simply
    // by seeing if it is < 0x80
    enum class Token : uint16_t {
        Bang        = int(m8r::Token::Bang),
        Percent     = int(m8r::Token::Percent),
        Ampersand   = int(m8r::Token::Ampersand),
        LParen      = int(m8r::Token::LParen),
        RParen      = int(m8r::Token::RParen),
        Star        = int(m8r::Token::Star),
        Plus        = int(m8r::Token::Plus),
        Comma       = int(m8r::Token::Comma),
        Minus       = int(m8r::Token::Minus),
        Period      = int(m8r::Token::Period),
        Slash       = int(m8r::Token::Slash),
        Colon       = int(m8r::Token::Colon),
        Semicolon   = int(m8r::Token::Semicolon),
        LT          = int(m8r::Token::LT),
        STO         = int(m8r::Token::STO),
        GT          = int(m8r::Token::GT),
        Question    = int(m8r::Token::Question),
        LBracket    = int(m8r::Token::LBracket),
        RBracket    = int(m8r::Token::RBracket),
        XOR         = int(m8r::Token::XOR),
        LBrace      = int(m8r::Token::LBrace),
        OR          = int(m8r::Token::OR),
        RBrace      = int(m8r::Token::RBrace),
        Twiddle     = int(m8r::Token::Twiddle),
        At          = int(m8r::Token::At),
        Dollar      = int(m8r::Token::Dollar),
        
        False       = int(m8r::Token::False),
        Null        = int(m8r::Token::Null),
        True        = int(m8r::Token::True),
        Undefined   = int(m8r::Token::Undefined),
        Unknown     = int(m8r::Token::Unknown),
        Comment     = int(m8r::Token::Comment),
        Whitespace  = int(m8r::Token::Whitespace),
        Float       = int(m8r::Token::Float),
        Identifier  = int(m8r::Token::Identifier),
        String      = int(m8r::Token::String),
        Integer     = int(m8r::Token::Integer),
        None        = int(m8r::Token::None),
        Special     = int(m8r::Token::Special),
        Error       = int(m8r::Token::Error),
        EndOfFile   = int(m8r::Token::EndOfFile),
        
        Break       = 0x181,
        Case        = 0x182,
        Class       = 0x183,
        Constructor = 0x184,
        Continue    = 0x185,
        Default     = 0x186,
        Delete      = 0x187,
        Do          = 0x188,
        Else        = 0x189,
        For         = 0x18a,
        Function    = 0x18b,
        If          = 0x18c,
        New         = 0x18d,
        Return      = 0x18e,
        Switch      = 0x18f,
        This        = 0x190,
        Var         = 0x191,
        While       = 0x192,

        ADDSTO      = 0x1a0,
        SUBSTO      = 0x1a1,
        MULSTO      = 0x1a2,
        DIVSTO      = 0x1a3,
        MODSTO      = 0x1a4,
        ANDSTO      = 0x1a5,
        ORSTO       = 0x1a6,
        XORSTO      = 0x1a7,
        LOR         = 0x1a8,
        LAND        = 0x1a9,
        SHLSTO      = 0x1aa,
        SHRSTO      = 0x1ab,
        SARSTO      = 0x1ac,

        EQ          = 0x1b0,       
        NE          = 0x1b1,       
        GE          = 0x1b2,       
        LE          = 0x1b3,       
        SHL         = 0x1b4,       
        SHR         = 0x1b5,       
        SAR         = 0x1b6,       
        INCR        = 0x1b7,       
        DECR        = 0x1b8,       
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
                uint32_t _token : 16;
                uint32_t _op : 8;
                uint32_t _prec : 5;
                uint32_t _assoc : 1;
                uint32_t _sto : 1;
            };
            uint32_t _u;
        };
    };
    
    // OperatorInfo is in Flash, so we need to access it as a single 4 byte read
    static_assert(sizeof(OperatorInfo) == 4, "OperatorInfo must fit in 4 bytes");

    enum class Expect { Expr, PropertyAssignment, Statement, DuplicateDefault, MissingVarDecl, OneVarDeclAllowed, ConstantValueRequired, While };

    bool expect(Token token);
    bool expect(Expect expect, bool expected = false, const char* = nullptr);
    
    void expectedError(Token token, const char* = nullptr);
    void expectedError(Expect expect, const char* = nullptr);
    
    Token getToken();
    const m8r::Scanner::TokenType& getTokenValue() { return _parser->_scanner.getTokenValue(); }
    void retireToken()
    {
        if (_retireScannerToken) {
            _parser->_scanner.retireToken();
        }
        _retireScannerToken = true;
        _currentToken = Token::None;
    }

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
    
    using CaseEntry = struct { Parser::Label toStatement; Parser::Label fromStatement; int32_t statementAddr; };

    bool caseClause(m8r::Vector<CaseEntry>& cases, int32_t &defaultStatement, 
                    Parser::Label& defaultFromStatementLabel, bool& haveDefault);

    bool classContents();
    uint32_t variableDeclarationList();
    bool variableDeclaration();
    
    uint32_t argumentList();
    void forLoopCondAndIt();
    void forIteration(m8r::Atom iteratorName);
    bool propertyAssignment();
    bool propertyName();
    void formalParameterList();
    

    bool primaryExpression();
    m8r::Mad<Function> functionExpression(bool ctor);
    bool classExpression();
    bool objectExpression();
    bool postfixExpression();
    bool unaryExpression();
    bool arithmeticExpression(uint8_t minPrec = 1);
    bool commaExpression();

    Parser* _parser;
    Token _currentToken = Token::None;
    bool _retireScannerToken = true;
    
    m8r::Vector<m8r::Vector<Parser::Label>> _breakStack;
    m8r::Vector<m8r::Vector<Parser::Label>> _continueStack;

    struct CompareTokens
    {
        int operator()(const Token& lhs, const Token& rhs) const { return static_cast<int>(lhs) - static_cast<int>(rhs); }
    };

    static OperatorInfo _opInfos[ ];
};

}
