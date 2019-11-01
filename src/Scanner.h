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

static constexpr uint8_t C_EOF = static_cast<uint8_t>(Token::EndOfFile);

class Scanner  {
public:
    typedef struct {
        m8r::Op             op;
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
