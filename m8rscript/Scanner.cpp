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

using namespace m8r;

static const char* specialSingleChar = "(),.:;?[]{}~";
static const char* specialFirstChar = "!%&*+-/<=>^|";

inline static bool findChar(const char* s, char c)
{
    while (*s) {
        if (*s++ == c) {
            return true;
        }
    }
    return false;
}

struct Keyword
{
	const char* word;
	Token token;
};

static Keyword keywords[] = {
	{ "break",		Token::Break },
	{ "case",		Token::Case },
	{ "continue",	Token::Continue },
	{ "default",	Token::Default },
	{ "delete",		Token::Delete },
	{ "do",			Token::Do },
	{ "else",		Token::Else },
	{ "for",		Token::For },
	{ "function",	Token::Function },
	{ "if",			Token::If },
	{ "new",		Token::New },
	{ "return",		Token::Return },
	{ "switch",		Token::Switch },
	{ "var",		Token::Var },
	{ "while",		Token::While },
};

static inline bool isDigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isOctal(uint8_t c)       { return c >= '0' && c <= '7'; }
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isDigit(c) || isIdFirst(c); }
static inline bool isWhitespace(uint8_t c)  { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }

// If the word is a keyword, return the token for it, otherwise return K_UNKNOWN
Token Scanner::scanKeyword(const char* s)
{
	for (int i = 0; i < sizeof(keywords) / sizeof(Keyword); ++i) {
		if (strcmp(keywords[i].word, s) == 0) {
			return keywords[i].token;
		}
	}
	return Token::Unknown;
}

Token Scanner::scanString(char terminal)
{
	uint8_t c;
	
	while ((c = get()) != C_EOF) {
		if (c == terminal) {
			break;
		}
        while (c == '\\') {
            switch((c = get())) {
                case 'a': c = 0x07; break;
                case 'b': c = 0x08; break;
                case 'f': c = 0x0c; break;
                case 'n': c = 0x0a; break;
                case 'r': c = 0x0d; break;
                case 't': c = 0x09; break;
                case 'v': c = 0x0b; break;
                case '\\': c = 0x5c; break;
                case '\'': c = 0x27; break;
                case '"': c = 0x22; break;
                case '?': c = 0x3f; break;
                case 'x': {
                    if ((c = get()) == C_EOF) {
                        return Token::String;
                    }
                    
                    if (!isHex(c) && !isDigit(c)) {
                        c = '?';
                        break;
                    }
                    
                    uint32_t num = 0;
                    putback(c);
                    while ((c = get()) != C_EOF) {
                        if (!isHex(c) && !isDigit(c)) {
                            break;
                        }
                        if (isDigit(c)) {
                            num = (num << 4) | (c - '0');
                        } else if (isUpper(c)) {
                            num = (num << 4) | ((c - 'A') + 0x0a);
                        } else {
                            num = (num << 4) | ((c - 'a') + 0x0a);
                        }
                    }
                    if (num > 0xffffff) {
                        _parser->addToString(static_cast<uint8_t>(num >> 24) & 0xff);
                        _parser->addToString(static_cast<uint8_t>(num >> 16) & 0xff);
                        _parser->addToString(static_cast<uint8_t>(num >> 8) & 0xff);
                        _parser->addToString(static_cast<uint8_t>(num) & 0xff);
                    } else if (num > 0xffff) {
                        _parser->addToString(static_cast<uint8_t>(num >> 16) & 0xff);
                        _parser->addToString(static_cast<uint8_t>(num >> 8) & 0xff);
                        _parser->addToString(static_cast<uint8_t>(num) & 0xff);
                    } else if (num > 0xff) {
                        _parser->addToString(static_cast<uint8_t>(num >> 8) & 0xff);
                        _parser->addToString(static_cast<uint8_t>(num) & 0xff);
                    } else {
                        _parser->addToString(static_cast<uint8_t>(num) & 0xff);
                    }
                    break;
                }
                default: {
                    if (!isOctal(c)) {
                        c = '?';
                        break;
                    }
                    
                    uint32_t size = 0;
                    uint32_t num = c - '0';
                    while ((c = get()) != C_EOF) {
                        if (!isOctal(c) || ++size >= 3) {
                            break;
                        }
                        num = (num << 3) | (c - '0');
                    }
                    _parser->addToString(static_cast<uint8_t>(num) & 0x3f);
                    break;
                }
            }
        }
		_parser->addToString(c);
	}
	return Token::String;
}

Token Scanner::scanSpecial()
{
	uint8_t c1 = get();
    uint8_t c2;
    if (c1 == C_EOF) {
        return Token::EndOfFile;
    }
    
    if (c1 == '<') {
        if ((c2 = get()) == C_EOF) {
            return Token::EndOfFile;
        }
        if (c2 == '<') {
            if ((c2 = get()) == C_EOF) {
                return Token::EndOfFile;
            }
            if (c2 == '=') {
                return Token::SHLSTO;
            }
            putback(c2);
            return Token::SHL;
        }
        if (c2 == '=') {
            return Token::LE;
        }
        putback(c2);
        return static_cast<Token>(c1);
    }

    if (c1 == '>') {
        if ((c2 = get()) == C_EOF) {
            return Token::EndOfFile;
        }
        if (c2 == '>') {
            if ((c2 = get()) == C_EOF) {
                return Token::EndOfFile;
            }
            if (c2 == '=') {
                return Token::SHRSTO;
            }
            if (c2 == '>') {
                if ((c2 = get()) == C_EOF) {
                    return Token::EndOfFile;
                }
                if (c2 == '=') {
                    return Token::SARSTO;
                }
                putback(c2);
                return Token::SAR;
            }
            putback(c2);
            return Token::SHR;
        }
        if (c2 == '=') {
            return Token::GE;
        }
        putback(c2);
        return static_cast<Token>(c1);
    }
        
    if (findChar(specialSingleChar, c1)) {
        return static_cast<Token>(c1);
    }
    
    if (!findChar(specialFirstChar, c1)) {
        putback(c1);
        return Token::EndOfFile;
    }

	if ((c2 = get()) == C_EOF) {
        return Token::EndOfFile;
    }
    
    switch(c2) {
        case '!':
            if (c2 == '=') {
                return Token::NE;
            }
            break;
        case '%':
            if (c2 == '=') {
                return Token::MODSTO;
            }
            break;
        case '&':
            if (c2 == '&') {
                return Token::LAND;
            }
            if (c2 == '=') {
                return Token::ANDSTO;
            }
            break;
        case '*':
            if (c2 == '=') {
                return Token::MULSTO;
            }
            break;
        case '+':
            if (c2 == '=') {
                return Token::ADDSTO;
            }
            if (c2 == '+') {
                return Token::INC;
            }
            break;
        case '-':
            if (c2 == '=') {
                return Token::SUBSTO;
            }
            if (c2 == '-') {
                return Token::DEC;
            }
            break;
        case '/':
            if (c2 == '=') {
                return Token::DIVSTO;
            }
            break;
        case '=':
            if (c2 == '=') {
                return Token::EQ;
            }
            break;
        case '^':
            if (c2 == '=') {
                return Token::XORSTO;
            }
            break;
        case '|':
            if (c2 == '=') {
                return Token::ORSTO;
            }
            if (c2 == '|') {
                return Token::LOR;
            }
            break;
    }
    putback(c2);
    return static_cast<Token>(c1);
}

Token Scanner::scanIdentifier()
{
	uint8_t c;
	_tokenString.erase();

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
        Token token = scanKeyword(_tokenString.c_str());
        return (token == Token::Unknown) ? Token::Identifier : token;
    }

    return Token::EndOfFile;
}

// Return the number of digits scanned
int32_t Scanner::scanDigits(int32_t& number, bool hex)
{
	uint8_t c;
    int32_t radix = hex ? 16 : 10;
    int32_t numDigits = 0;
    
	while ((c = get()) != C_EOF) {
		if (isDigit(c)) {
            number = number * radix;
            number += static_cast<int32_t>(c - '0');
        } else if (isLCHex(c)) {
            number = number * radix;
            number += static_cast<int32_t>(c - 'a' + 10);
        } else if (isUCHex(c)) {
            number = number * radix;
            number += static_cast<int32_t>(c - 'A' + 10);
        } else {
			putback(c);
			break;
        }
        ++numDigits;
	}
    return numDigits;
}

Token Scanner::scanNumber(TokenType& tokenValue)
{
	uint8_t c = get();
    if (c == C_EOF) {
        return Token::EndOfFile;
    }
    
	if (!isDigit(c)) {
		putback(c);
		return Token::EndOfFile;
	}
	
    bool hex = false;
    int32_t number = static_cast<int32_t>(c - '0');
    int32_t exp = 0;

    if (c == '0') {
        if ((c = get()) == C_EOF) {
            return Token::EndOfFile;
        }
        if (c == 'x' || c == 'X') {
            if ((c = get()) == C_EOF) {
                return Token::EndOfFile;
            }
            if (!isDigit(c)) {
                putback(c);
                return Token::Unknown;
            }
            hex = true;
        }
        putback(c);
	}
    
    scanDigits(number, hex);
    if (scanFloat(number, exp)) {
        Float f(number, exp);
        tokenValue.number = static_cast<Float::Raw>(f);
        return Token::Float;
    }
    assert(exp == 0);
    tokenValue.integer = static_cast<uint32_t>(number);
    return Token::Integer;
}

bool Scanner::scanFloat(int32_t& mantissa, int32_t& exp)
{
    bool haveFloat = false;
	uint8_t c;
    if ((c = get()) == C_EOF) {
        return false;
    }
    if (c == '.') {
        haveFloat = true;
        exp = -scanDigits(mantissa, false);
        if ((c = get()) == C_EOF) {
            return false;
        }
    }
    if (c == 'e' || c == 'E') {
        haveFloat = true;
        if ((c = get()) == C_EOF) {
            return false;
        }
        int32_t neg = 1;
        if (c == '+' || c == '-') {
            if (c == '-') {
                neg = -1;
            }
        } else {
            putback(c);
        }
        int32_t realExp;
        scanDigits(realExp, false);
        exp += neg * realExp;
    } else {
        putback(c);
    }
    return haveFloat;
}

Token Scanner::scanComment()
{
	uint8_t c;

	if ((c = get()) == '*') {
		for ( ; ; ) {
			c = get();
			if (c == C_EOF) {
				return Token::EndOfFile;
			}
			if (c == '*') {
				if ((c = get()) == '/') {
					break;
				}
				putback(c);
			}
		}
		return Token::Comment;
	}
	if (c == '/') {
		// Comment
		for ( ; ; ) {
			c = get();
			if (c == C_EOF) {
				return Token::EndOfFile;
			}
			if (c == '\n') {
				break;
			}
		}
		return Token::Comment;
	}
	return static_cast<Token>('/');
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

Token Scanner::getToken(TokenType& tokenValue)
{
	uint8_t c;
	Token token = Token::EndOfFile;
	
	while (token == Token::EndOfFile && (c = get()) != C_EOF) {
        if (isWhitespace(c)) {
            continue;
        }
		switch(c) {
			case '/':
				token = scanComment();
				if (token == Token::Comment) {
					// For now we ignore comments
                    token = Token::EndOfFile;
					break;
				}
				break;
				
			case '\"':
			case '\'':
                tokenValue.string = _parser->startString();
				token = scanString(c);
                _parser->endString();
				break;

			default:
				putback(c);
				if ((token = scanNumber(tokenValue)) != Token::EndOfFile) {
					break;
				}
				if ((token = scanSpecial()) != Token::EndOfFile) {
					break;
				}
				if ((token = scanIdentifier()) != Token::EndOfFile) {
                    if (token == Token::Identifier) {
                        tokenValue.atom = _parser->atomizeString(_tokenString.c_str());
                        _tokenString.erase();
                    }
					break;
				}
				token = Token::Unknown;
                break;
		}
	}
    
	return token;
}
