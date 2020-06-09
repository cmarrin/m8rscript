/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MStream.h"
#include "Float.h"
#include "Defines.h"

namespace m8r {
    class Function;
}

#define MAX_ID_LENGTH 32

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Scanner
//
//  
//
//////////////////////////////////////////////////////////////////////////////

#undef DEC
enum class Token : uint8_t {
    Break       = 0x01,
    Case        = 0x02,
    Class       = 0x03,
    Constructor = 0x04,
    Continue    = 0x05,
    Default     = 0x06,
    Delete      = 0x07,
    Do          = 0x08,
    Else        = 0x09,
    False       = 0x0a,
    For         = 0x0b,
    Function    = 0x0c,
    If          = 0x0d,
    New         = 0x0e,
    Null        = 0x0f,
    Return      = 0x10,
    Switch      = 0x11,
    This        = 0x12,
    True        = 0x13,
    Undefined   = 0x14,
    Var         = 0x15,
    While       = 0x016,
    
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
    
    SHRSTO      = 0x80,
    SARSTO      = 0x81,
    SHLSTO      = 0x82,
    ADDSTO      = 0x83,
    SUBSTO      = 0x84,
    MULSTO      = 0x85,
    DIVSTO      = 0x86,
    MODSTO      = 0x87,
    ANDSTO      = 0x88,
    XORSTO      = 0x89,
    ORSTO       = 0x8a,
    SHR         = 0x8b,
    SAR         = 0x8c,
    SHL         = 0x8d,
    INC         = 0x8e,
    DEC         = 0x8f,
    LAND        = 0x90,
    LOR         = 0x91,
    LE          = 0x92,
    GE          = 0x93,
    EQ          = 0x94,
    NE          = 0x95,
    
    Unknown     = 0xc0,
    Comment     = 0xc1,
    Whitespace  = 0xc2,

    Float       = 0xd0,
    Identifier  = 0xd1,
    String      = 0xd2,
    Integer     = 0xd3,
    
    Expr        = 0xe0,
    
    PropertyAssignment      = 0xe1,
    Statement               = 0xe2,
    DuplicateDefault        = 0xe3,
    MissingVarDecl          = 0xe4,
    OneVarDeclAllowed       = 0xe5,
    ConstantValueRequired   = 0xe6,

    None        = 0xfd,
    Error       = 0xfe,
    EndOfFile   = 0xff,
};

static constexpr uint8_t C_EOF = static_cast<uint8_t>(Token::EndOfFile);

struct Label {
    int32_t label : 20;
    uint32_t uniqueID : 12;
    int32_t matchedAddr : 20;
};

class Scanner  {
public:
    typedef struct {
        m8r::Label          label;
        m8r::Function*      function;
        m8r::Float::Raw		number;
        uint32_t            integer;
        uint32_t            argcount;
        const char*         str;
    } TokenType;

  	Scanner(m8r::Stream* istream = nullptr)
  	 : _lastChar(C_EOF)
  	 , _istream(istream)
     , _lineno(1)
  	{
    }
  	
  	~Scanner()
  	{
    }
    
    void setStream(const m8r::Stream* istream) { _istream = istream; }
  
    uint32_t lineno() const { return _lineno; }
  	
  	Token getToken(TokenType& token, bool ignoreWhitespace = true);

    Token getToken()
    {
        if (_currentToken == Token::None) {
            _currentToken = getToken(_currentTokenValue);
        }
        return _currentToken;
    }
    
    const Scanner::TokenType& getTokenValue()
    {
        if (_currentToken == Token::None) {
            _currentToken = getToken(_currentTokenValue);
        }
        return _currentTokenValue;
    }
    
    void retireToken() { _currentToken = Token::None; }

private:
    uint8_t get() const;
    
	void putback(uint8_t c) const
	{
  		assert(_lastChar == C_EOF && c != C_EOF);
  		_lastChar = c;
	}

  	Token scanKeyword(const char*);
  	Token scanString(char terminal);
  	Token scanSpecial();
  	Token scanIdentifier();
  	Token scanNumber(TokenType& tokenValue);
  	Token scanComment();
  	int32_t scanDigits(int32_t& number, bool hex);
  	bool scanFloat(int32_t& mantissa, int32_t& exp);
    
  	mutable uint8_t _lastChar;
  	m8r::String _tokenString;
  	const m8r::Stream* _istream;
    mutable uint32_t _lineno;

    Token _currentToken = Token::None;
    Scanner::TokenType _currentTokenValue;
};

}
