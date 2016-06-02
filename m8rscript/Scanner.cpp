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

#include "Scanner.h"

#include <cstdio>
#include <cstring>

using namespace m8r;

struct Keyword
{
	const char* word;
	int token;
};

static Keyword keywords[] = {
	{ "!", 			'!' },
	{ "!=", 		O_NE },
	{ "%", 			'%' },
	{ "%=", 		O_MODEQ },
	{ "&", 			'&' },
	{ "&&", 		O_LAND },
	{ "&=", 		O_ANDEQ },
	{ "(",			'(' },
	{ ")",			')' },
	{ "*", 			'*' },
	{ "*=", 		O_MULEQ },
	{ "+", 			'+' },
	{ "++", 		O_INC },
	{ "+=", 		O_ADDEQ },
	{ ",",			',' },
	{ "-", 			'-' },
	{ "--", 		O_DEC },
	{ "-=", 		O_SUBEQ },
	{ "/", 			'/' },
	{ "/=", 		O_DIVEQ },
	{ ":",			':' },
	{ ";",			';' },
	{ "<", 			'<' },
	{ "<<", 		O_LSHIFT },
	{ "<=", 		O_LE },
	{ "<<=", 		O_LSHIFTEQ },
	{ "=",			'=' },
	{ "==", 		O_EQ },
	{ ">", 			'>'},
	{ ">=", 		O_GE },
	{ ">>", 		O_RSHIFT },
	{ ">>=", 		O_RSHIFTEQ },
	{ ">>>", 		O_RSHIFTFILL },
	{ ">>>=", 		O_RSHIFTFILLEQ },
	{ "?",			'?' },
	{ "[",			'[' },
	{ "]",			']' },
	{ "^", 			'^' },
	{ "^=", 		O_XOREQ },
	{ "{",			'{' },
	{ "|", 			'|' },
	{ "|=", 		O_OREQ },
	{ "||", 		O_LOR },
	{ "}",			'}' },
	{ "~",			'~' },
	{ "break",		K_BREAK },
	{ "case",		K_CASE },
	{ "continue",	K_CONTINUE },
	{ "default",	K_DEFAULT },
	{ "delete",		K_DELETE },
	{ "do",			K_DO },
	{ "else",		K_ELSE },
	{ "for",		K_FOR },
	{ "function",	K_FUNCTION },
	{ "if",			K_IF },
	{ "new",		K_NEW },
	{ "return",		K_RETURN },
	{ "switch",		K_SWITCH },
	{ "var",		K_VAR },
	{ "while",		K_WHILE },
};

/*
	invalid chars: '#', '\'
	"!"		0x21
	"%"		0x25
	"&"		0x26
	"("		0x28
	")"		0x29
	"*"		0x2a
	"+"		0x2b
	","		0x2c
	"-"		0x2d
	"."		0x2e
	"/"		0x2f
	
	":"		0x3a
	";"		0x3b
	"<"		0x3c
	"="		0x3d
	">"		0x3e
	"?"		0x3f
	
	"["		0x5b
	"]"		0x5d
	"^"		0x5e
	
	"{"		0x7b
	"|"		0x7c
	"}"		0x7d
	"~"		0x7e
	
;
*/

static inline bool isDigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isHex(uint8_t c)	{ return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isDigit(c) || isIdFirst(c); }
static inline bool isSpecial(uint8_t c)		{ return !isDigit(c) && !isIdFirst(c) && c >= 0x21 && c <= 0x7e; }

void Scanner::printError(const char* s)
{
	printf("%s\n", s);
}

// If the word is a keyword, return the token for it, otherwise return T_IDENTIFIER
uint8_t Scanner::scanKeyword(uint32_t current, uint32_t len)
{
	for (int i = 0; i < sizeof(keywords) / sizeof(Keyword); ++i) {
		if (strncmp(keywords[i].word, &(_ostring[current]), len) == 0) {
			return keywords[i].token;
		}
	}
	return C_EOF;
}

uint8_t Scanner::scanString(char terminal)
{
	uint8_t c;
	
	while ((c = _istream->read()) != C_EOF) {
		if (c == terminal) {
			_ostring += '\0';
			break;
		}
		_ostring += c;
	}
	return T_STRING;
}

uint8_t Scanner::scanSpecial()
{
	uint8_t c;
	uint32_t current = _ostring.length();
	
	while ((c = _istream->read()) != C_EOF) {
		if (!isSpecial(c)) {
			_ostring += '\0';
			putback(c);
			break;
		}
		_ostring += c;
    }

    uint32_t len = _ostring.length() - current;
    if (!len) {
        return C_EOF;
    }
    uint8_t token = scanKeyword(current, len);

	return (token == C_EOF) ? K_UNKNOWN : token;
}

uint8_t Scanner::scanIdentifier()
{
	uint8_t c;
	uint32_t current = _ostring.length();

	bool first = true;
	while ((c = _istream->read()) != C_EOF) {
		if (!((first && isIdFirst(c)) || (!first && isIdOther(c)))) {
			_ostring += '\0';
			putback(c);
			break;
		}
		_ostring += c;
		first = false;
	}
    uint32_t len = _ostring.length() - current;
    return (len) ? scanKeyword(current, len) : C_EOF;
}

void Scanner::scanDigits(bool hex)
{
	uint8_t c;
	while ((c = _istream->read()) != C_EOF) {
		if (!isDigit(c) || (hex && isHex(c))) {
			_ostring += '\0';
			putback(c);
			break;
		}
		_ostring += c;
	}
}

uint8_t Scanner::scanNumber()
{
	uint8_t c;
    bool hex = false;
	
	if (!isDigit(c = _istream->read())) {
		putback(c);
		return C_EOF;
	}
	
	_ostring += c;
	
	if (c == '0' && ((c = _istream->read()) == 'x' || c == 'X')) {
        _ostring += 'x';
		if (!isDigit(c = _istream->read())) {
			putback(c);
			return K_UNKNOWN;
		}
        hex = true;
	} else {
		putback(c);
	}
	
	scanDigits(hex);
	return T_INTEGER;
}

uint8_t Scanner::scanComment()
{
	uint8_t c;

	if ((c = _istream->read()) == '*') {
		for ( ; ; ) {
			c = _istream->read();
			if (c == C_EOF) {
				return C_EOF;
			}
			if (c == '*') {
				if ((c = _istream->read()) == '/') {
					break;
				}
				putback(c);
			}
		}
		return K_COMMENT;
	}
	if (c == '/') {
		// Comment
		for ( ; ; ) {
			c = _istream->read();
			if (c == C_EOF) {
				return C_EOF;
			}
			if (c == '\n') {
				break;
			}
		}
		return K_COMMENT;
	}
	return '/';
}

uint8_t Scanner::getToken(TokenValue* tokenValue)
{
	uint8_t c;
	uint8_t token;
	
	while ((c = _istream->read()) != C_EOF) {
		switch(c) {
			case '/':
				token = scanComment();
				if (token == K_COMMENT) {
					// For now we ignore comments
					break;
				}
				return token;
				
			case '\"':
				return scanString('\"');

			case '\'':
				return scanString('\'');
			
			default:
				putback(c);
				if ((token = scanNumber()) != C_EOF) {
					return token;
				}
				if ((token = scanSpecial()) != C_EOF) {
					return token;
				}
				if ((token = scanIdentifier()) != C_EOF) {
					return token;
				}
				return K_UNKNOWN;
		}
	}
	return C_EOF;
}

#if 0
void
Scanner::getToken(TokenValue& token)
{
	uint8_t c = (_lastChar == C_EOF) ? getChar() : _lastChar;
	_lastChar = C_EOF;
	token.token = C_EOF;
	
	if (c == C_EOF)
		return;
	
	if (isDigit(c)) {
		// scanning a number <digit>+['.'<digit>*]?[['E'|'e']['+'|'-']?<digit>+]?
		//            state:    0      1     2         3       4        5         6
		// or
		//                   '0'['x'|'X']<hexdigit>+
		uint8_t c1 = c;
		c = getChar();
		if (c == C_EOF)
			return;
		
		// check for hex
		if (c1 == '0' && (c == 'x' || c == 'X')) {
			int32_t n = 0;
			while (isHexDigit(c = getChar())) {
				n *= 16;
				n += isDigit(c) ? (c-'0') : (toLower(c)-'a'+10);
			}
			
			if (c != C_EOF)
				_lastChar = c;
				
			token.integer = n;
			token.token = T_INTEGER;
			return;
		}
		else {
			uint8_t state = 0;
			bool needChar = false;
			uint32_t i = 0;
			uint32_t f = 0;
			uint8_t dp = 0;
			int32_t e = 0;
			bool negExp = false;
			
			while (1) {
				switch(state) {
					case 0:	// digits before decimal
						if (isDigit(c)) {
							i = i*10+(c-'0');
							needChar = true;
						}
						else
							state = 1;
						break;
					case 1: // decimal point
						if (c == '.') {
							needChar = true;
							state = 2;
						}
						else
							state = 3;
						break;
					case 2: // digits after decimal
						if (isDigit(c)) {
							f = f*10+(c-'0');
							dp++;
							needChar = true;
						}
						else
							state = 3;
						break;
					case 3: // exponent
						if (c == 'e' || c == 'E') {
							needChar = true;
							state = 4;
						}
						else
							state = 6;
						break;
					case 4: // exponent + or -
						if (c == '+' || c == '-') {
							if (c == '-')
								negExp = true;
							needChar = true;
						}
						state = 5;
						break;
					case 5: // exponent number
						if (isDigit(c)) {
							e = e*10+(c-'0');
							needChar = true;
						}
						else
							state = 6;
						break;
				}
				
				if (needChar) {
					needChar = false;
					c = getChar();
					if (c == C_EOF) {
						if (state == 0 || state == 2 || state == 5)
							state = 6;
						else {
							token.token = E_ERROR;
							return;
						}
					}
				}
				
				if (state == 6) {
					if (e == 0 && f == 0 && dp == 0) {
						// return integer
						token.integer = i;
						token.token = T_INTEGER;
						return;
					}
					else {
						if (negExp)
							e = -e;
						token.number = FPF_MAKE(i, f, dp, e);
						token.token = T_FLOAT;
						return;
					}
				}
			}
		}
	}
	else if (isIdFirst(c)) {
		// scanning identifier or keyword <idfirst><idother>*
		_tokenString.resize(MAX_ID_LENGTH);
		char* p = &(_tokenString[0]);
		uint16_t n = MAX_ID_LENGTH;
	
		while (isIdOther(c = getChar()) && n-- > 1)
			*p++ = (char) c;
			
		if (c != C_EOF)
			_lastChar = c;
			
		// make it an atom
		const char* s = &(_tokenString[0]);
		uint16_t len = p-s;
		
		// see if this is a keyword
		uint8_t keyword = scanKeyword(p);
		if (keyword != C_EOF)
			token.token = keyword;
		else {
			token.atom = _atomList->add(s, len);
			token.token = T_IDENTIFIER;
		}
		return;
	}
	else if (c == '\'' || c == '"') {
		// scan string
		char* p = &(_tokenString[0]);
		uint8_t match = c;
		
		--p;	// toss quote
		
		while(1) {
			c = getChar();
			if (c != match) {
				if (c == '\\') {
					c = getChar();
					switch(c) {
						case 'n':	c = '\n'; break;
						case 't':	c = '\t'; break;
						case 'r':	c = '\r'; break;
						case '\'':	c = '\''; break;
						case '"':	c = '"'; break;
						// handle the rest later
					}
				}
				
				uint16_t n = p-&(_tokenString[0]);
				if (_tokenString.size() < n - 2)
					_tokenString.resize(_tokenString.size() + MAX_ID_LENGTH);
					
				p = &(_tokenString[n]);
				*p++ = (char) c;
			}
		}
		
		char* s = &(_tokenString[0]);
		token.atom = _atomList->add(s, p-s);
		token.token = T_STRING;
		return;
	}
	else if (isSpecial(c)) {
		// scanning special char sequence <special>+
		char s[5];
		char* p = s;
		uint8_t n = 4;
		*p++ = c;
		
		while (isSpecial(c = getChar()) && n-- > 0)
			*p++ = (char) c;
		
		*p = '\0';
		
		if (c != C_EOF)
			_lastChar = c;
			
		if (s[1] == '\0') {
			// single character case
			token.token = (s[0] == '#' || s[0] == '\\') ? C_EOF : s[0];
			return;
		}

		token.token = scanKeyword( &(s[0]));
		return;
	}
	
	token.token = C_EOF;
}
#endif
