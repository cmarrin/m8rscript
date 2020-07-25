/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Float.h"
#include "MStream.h"
#include "MString.h"

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

// Tokens can't be a special char, so avoid 0x00 - 0x7f
enum class Token : uint8_t {
    False       = 0x80,
    Null        = 0x81,
    True        = 0x82,
    Undefined   = 0x83,
    Unknown     = 0x84,
    Comment     = 0x85,
    Whitespace  = 0x86,
    Float       = 0x87,
    Identifier  = 0x88,
    String      = 0x89,
    Integer     = 0x8a,
    None        = 0x8b,
    Special     = 0x8c,
    Error       = 0x8d,
    EndOfFile   = 0x8e,

    Bang        = 0x90,
    Percent     = 0x91,
    Ampersand   = 0x92,
    LParen      = 0x93,
    RParen      = 0x94,
    Star        = 0x95,
    Plus        = 0x96,
    Comma       = 0x97,
    Minus       = 0x98,
    Period      = 0x99,
    Slash       = 0x9a,
    Colon       = 0x9b,
    Semicolon   = 0x9c,
    LT          = 0x9d,
    STO         = 0x9e,
    GT          = 0x9f,
    Question    = 0xa0,
    LBracket    = 0xa1,
    RBracket    = 0xa2,
    XOR         = 0xa3,
    LBrace      = 0xa4,
    OR          = 0xa5,
    RBrace      = 0xa6,
    Twiddle     = 0xa7,
    
    SHRSTO      = 0xb1,
    SARSTO      = 0xb2,
    SHLSTO      = 0xb3,
    ADDSTO      = 0xb4,
    SUBSTO      = 0xb5,
    MULSTO      = 0xb6,
    DIVSTO      = 0xb7,
    MODSTO      = 0xb8,
    ANDSTO      = 0xb9,
    XORSTO      = 0xba,
    ORSTO       = 0xbb,
    SHR         = 0xbc,
    SAR         = 0xbd,
    SHL         = 0xbe,
    INCR        = 0xbf,
    DECR        = 0xc0,
    LAND        = 0xc1,
    LOR         = 0xc2,
    LE          = 0xc3,
    GE          = 0xc4,
    EQ          = 0xc5,
    NE          = 0xc6,
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
        Label           label;
        Function*       function;
        Float::Raw	    number;
        uint32_t        integer;
        uint32_t        argcount;
        const char*     str;
    } TokenType;

  	Scanner(Stream* istream = nullptr)
  	 : _lastChar(C_EOF)
  	 , _istream(istream)
     , _lineno(1)
  	{
    }
  	
  	~Scanner()
  	{
    }
    
    void setStream(const Stream* istream) { _istream = istream; }
  
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

  	Token scanString(char terminal);
  	Token scanSpecial();
  	Token scanIdentifier();
  	Token scanNumber(TokenType& tokenValue);
  	Token scanComment();
  	int32_t scanDigits(int32_t& number, bool hex);
  	bool scanFloat(int32_t& mantissa, int32_t& exp);
    
  	mutable uint8_t _lastChar;
  	String _tokenString;
  	const Stream* _istream;
    mutable uint32_t _lineno;

    Token _currentToken = Token::None;
    Scanner::TokenType _currentTokenValue;
};

}
