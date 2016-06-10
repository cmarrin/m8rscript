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

#include "Parser.h"

#include <cstring>

using namespace m8r;

static const char* specialSingleChar = "(),.:;?[]{}~";
static const char* specialFirstChar = "!%&*+-/<=>^|";

struct Keyword
{
	const char* word;
	int token;
};

static Keyword keywords[] = {
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

static inline bool isDigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isHex(uint8_t c)         { return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isDigit(c) || isIdFirst(c); }
static inline bool isWhitespace(uint8_t c)  { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }

// If the word is a keyword, return the token for it, otherwise return K_UNKNOWN
uint8_t Scanner::scanKeyword(const char* s)
{
	for (int i = 0; i < sizeof(keywords) / sizeof(Keyword); ++i) {
		if (strcmp(keywords[i].word, s) == 0) {
			return keywords[i].token;
		}
	}
	return K_UNKNOWN;
}

uint8_t Scanner::scanString(char terminal)
{
	uint8_t c;
    _tokenString.clear();
	
	while ((c = get()) != C_EOF) {
		if (c == terminal) {
			break;
		}
		_tokenString += c;
	}
	return T_STRING;
}

uint8_t Scanner::scanSpecial()
{
	uint8_t c1 = get();
    uint8_t c2;
    if (c1 == C_EOF) {
        return C_EOF;
    }
    
    if (c1 == '<') {
        if ((c2 = get()) == C_EOF) {
            return C_EOF;
        }
        if (c2 == '<') {
            if ((c2 = get()) == C_EOF) {
                return C_EOF;
            }
            if (c2 == '=') {
                return O_LSHIFTEQ;
            }
            putback(c2);
            return O_LSHIFT;
        }
        if (c2 == '=') {
            return O_LE;
        }
        putback(c2);
        return c1;
    }

    if (c1 == '>') {
        if ((c2 = get()) == C_EOF) {
            return C_EOF;
        }
        if (c2 == '>') {
            if ((c2 = get()) == C_EOF) {
                return C_EOF;
            }
            if (c2 == '=') {
                return O_RSHIFTEQ;
            }
            if (c2 == '>') {
                if ((c2 = get()) == C_EOF) {
                    return C_EOF;
                }
                if (c2 == '=') {
                    return O_RSHIFTFILLEQ;
                }
                putback(c2);
                return O_RSHIFTFILL;
            }
            putback(c2);
            return O_RSHIFT;
        }
        if (c2 == '=') {
            return O_GE;
        }
        putback(c2);
        return c1;
    }
        
    if (strchr(specialSingleChar, c1)) {
        return c1;
    }
    
    if (!strchr(specialFirstChar, c1)) {
        putback(c1);
        return C_EOF;
    }

	if ((c2 = get()) == C_EOF) {
        return C_EOF;
    }
    
    switch(c2) {
        case '!':
            if (c2 == '=') {
                return O_NE;
            }
            break;
        case '%':
            if (c2 == '=') {
                return O_MODEQ;
            }
            break;
        case '&':
            if (c2 == '&') {
                return O_LAND;
            }
            if (c2 == '=') {
                return O_ANDEQ;
            }
            break;
        case '*':
            if (c2 == '=') {
                return O_MULEQ;
            }
            break;
        case '+':
            if (c2 == '=') {
                return O_ADDEQ;
            }
            if (c2 == '+') {
                return O_INC;
            }
            break;
        case '-':
            if (c2 == '=') {
                return O_SUBEQ;
            }
            if (c2 == '-') {
                return O_DEC;
            }
            break;
        case '/':
            if (c2 == '=') {
                return O_DIVEQ;
            }
            break;
        case '=':
            if (c2 == '=') {
                return O_EQ;
            }
            break;
        case '^':
            if (c2 == '=') {
                return O_XOREQ;
            }
            break;
        case '|':
            if (c2 == '=') {
                return O_OREQ;
            }
            if (c2 == '|') {
                return O_LOR;
            }
            break;
    }
    putback(c2);
    return c1;
}

uint8_t Scanner::scanIdentifier()
{
	uint8_t c;
	_tokenString.clear();

	bool first = true;
	while ((c = get()) != C_EOF) {
		if (!((first && isIdFirst(c)) || (!first && isIdOther(c)))) {
			putback(c);
			break;
		}
		_tokenString += c;
		first = false;
	}
    size_t len = _tokenString.length();
    if (len) {
        uint8_t token = scanKeyword(_tokenString.c_str());
        return (token == K_UNKNOWN) ? T_IDENTIFIER : token;
    }

    return C_EOF;
}

void Scanner::scanDigits(bool hex)
{
	uint8_t c;
	while ((c = get()) != C_EOF) {
		if (!isDigit(c) || (hex && isHex(c))) {
			putback(c);
			break;
		}
		_tokenString += c;
	}
}

uint8_t Scanner::scanNumber()
{
	_tokenString.clear();
    
	uint8_t c = get();
    if (c == C_EOF) {
        return C_EOF;
    }
    bool hex = false;
	
	if (!isDigit(c)) {
		putback(c);
		return C_EOF;
	}
	
	_tokenString += c;
    
    if (c == '0') {
        if ((c = get()) == C_EOF) {
            return C_EOF;
        }
        if (c == 'x' || c == 'X') {
            _tokenString += 'x';
            if ((c = get()) == C_EOF) {
                return C_EOF;
            }
            if (!isDigit(c)) {
                putback(c);
                return K_UNKNOWN;
            }
            hex = true;
        }
        putback(c);
	}
    
    scanDigits(hex);
    return scanFloat() ? T_FLOAT : T_INTEGER;
}

bool Scanner::scanFloat()
{
    bool haveFloat = false;
	uint8_t c;
    if ((c = get()) == C_EOF) {
        return false;
    }
    if (c == '.') {
        haveFloat = true;
        _tokenString += c;
        scanDigits(false);
        if ((c = get()) == C_EOF) {
            return false;
        }
    }
    if (c == 'e' || c == 'E') {
        haveFloat = true;
        _tokenString += 'e';
        if ((c = get()) == C_EOF) {
            return false;
        }
        if (c == '+' || c == '-') {
            _tokenString += c;
        } else {
            putback(c);
        }
        scanDigits(false);
    } else {
        putback(c);
    }
    return haveFloat;
}

uint8_t Scanner::scanComment()
{
	uint8_t c;

	if ((c = get()) == '*') {
		for ( ; ; ) {
			c = get();
			if (c == C_EOF) {
				return C_EOF;
			}
			if (c == '*') {
				if ((c = get()) == '/') {
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
			c = get();
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

uint8_t Scanner::get() const
{
    if (_lastChar != C_EOF) {
        uint8_t c = _lastChar;
        _lastChar = C_EOF;
        return c;
    }
    if (!_istream->available()) {
        return C_EOF;
    }
    uint8_t c = _istream->read();
    if (c == '\n') {
        ++_lineno;
    }
    return c;
}

uint8_t Scanner::getToken(YYSTYPE* tokenValue)
{
	uint8_t c;
	uint8_t token = C_EOF;
	
	while (token == C_EOF && (c = get()) != C_EOF) {
        if (isWhitespace(c)) {
            continue;
        }
		switch(c) {
			case '/':
				token = scanComment();
				if (token == K_COMMENT) {
					// For now we ignore comments
                    token = C_EOF;
					break;
				}
				break;
				
			case '\"':
			case '\'':
				token = scanString(c);
                tokenValue->string = _parser->addString(_tokenString.c_str());
                _tokenString.clear();
				break;

			default:
				putback(c);
				if ((token = scanNumber()) != C_EOF) {
                    if (token == T_INTEGER) {
                        tokenValue->integer = static_cast<uint32_t>(strtol(_tokenString.c_str(), NULL, 0));
                    } else {
                        tokenValue->number = static_cast<float>(atof(_tokenString.c_str()));
                    }
                    _tokenString.clear();
					break;
				}
				if ((token = scanSpecial()) != C_EOF) {
					break;
				}
				if ((token = scanIdentifier()) != C_EOF) {
                    if (token == T_IDENTIFIER) {
                        tokenValue->atom = _parser->atomizeString(_tokenString.c_str());
                        _tokenString.clear();
                    }
					break;
				}
				token = K_UNKNOWN;
                break;
		}
	}
    
	return token;
}
