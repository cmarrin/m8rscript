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
#include "MString.h"

#define MAX_ID_LENGTH 32

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Scanner
//
//  
//
//////////////////////////////////////////////////////////////////////////////

// Tokens for special chars are the same as their ASCII code. Other tokens 
// have values above 0x7f. Script parsers rely on these facts. Don't change 
// them unless you change those other places.
enum class Token : uint8_t {
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
};

static constexpr uint8_t C_EOF = static_cast<uint8_t>(Token::EndOfFile);

class Scanner  {
public:
    typedef struct {
        float   	    number;
        uint32_t        integer;
        uint32_t        argcount;
        const char*     str;
    } TokenType;

  	Scanner(const Stream* istream = nullptr)
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
  	
    Token getToken(bool ignoreWhitespace = true)
    {
        if (_currentToken == Token::None) {
            _currentToken = getToken(_currentTokenValue, ignoreWhitespace);
        }
        return _currentToken;
    }
    
    const Scanner::TokenType& getTokenValue(bool ignoreWhitespace = true)
    {
        if (_currentToken == Token::None) {
            _currentToken = getToken(_currentTokenValue, ignoreWhitespace);
        }
        return _currentTokenValue;
    }
    
    void retireToken() { _currentToken = Token::None; }

private:
  	Token getToken(TokenType& token, bool ignoreWhitespace);

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
