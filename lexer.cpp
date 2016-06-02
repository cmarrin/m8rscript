#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "parse.tab.h"

#define BUFFER_SIZE 256

char buffer[BUFFER_SIZE];

FILE* infile;

typedef struct {
	const char* word;
	int token;
} Keyword;

Keyword keywords[] = {
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

int next = -1;

int getNextChar()
{
	if (next != -1) {
		int r = next;
		next = -1;
		return r;
	}
	return getc(infile);
}

void putback(int c)
{
	assert(next == -1);
	next = c;
}

int scanString(char terminal)
{
	char* p = buffer;
	int c;
	while ((c = getNextChar()) != EOF) {
		if (c == terminal) {
			*p = '\0';
			break;
		}
		if (p - buffer < BUFFER_SIZE - 1) {
			*p++ = c;
		}
	}
	return T_STRING;
}

int scanSpecial()
{
	char* p = buffer;
	int c;
	while ((c = getNextChar()) != EOF) {
		if (!ispunct(c)) {
			*p = '\0';
			putback(c);
			break;
		}
		if (p - buffer < BUFFER_SIZE - 1) {
			*p++ = c;
		}
	}
	return (buffer == p) ? -1 : K_UNKNOWN;
}

int scanIdentifier()
{
	char* p = buffer;
	int c;
	bool first = true;
	while ((c = getNextChar()) != EOF) {
		if (!((first && isalpha(c)) || (!first && isalnum(c)))) {
			*p = '\0';
			putback(c);
			break;
		}
		if (p - buffer < BUFFER_SIZE - 1) {
			*p++ = c;
		}
		first = false;
	}
	return (buffer == p) ? -1 : T_IDENTIFIER;
}

void scanDigits(char* p)
{
	int c;
	while ((c = getNextChar()) != EOF) {
		if (!isdigit(c)) {
			*p = '\0';
			putback(c);
			break;
		}
		if (p - buffer < BUFFER_SIZE - 1) {
			*p++ = c;
		}
	}
}

int scanNumber()
{
	char* p = buffer;
	int c;
	if (!isdigit(c = getNextChar())) {
		putback(c);
		return -1;
	}
	
	*p++ = c;
	
	if (c == '0' && ((c = getNextChar()) == 'x' || c == 'X')) {
		*p++ = 'x';
		if (!isdigit(c = getNextChar())) {
			putback(c);
			return K_UNKNOWN;
		}
	} else {
		putback(c);
	}
	
	scanDigits(p);
	return T_INTEGER;
}

// If the word is a keyword, return the token for it, otherwise return T_IDENTIFIER
int scanKeyword(const char* word)
{
	for (int i = 0; i < sizeof(keywords) / sizeof(Keyword); ++i) {
		if (strcmp(keywords[i].word, word) == 0) {
			return keywords[i].token;
		}
	}
	return T_IDENTIFIER;
}

int yylex()
{
	int c;
	int token;
	
	while ((c = getNextChar()) != EOF) {
		switch(c) {
			case '/':
				if ((c = getNextChar()) == '*') {
					// Comment
					for ( ; ; ) {
						c = getNextChar();
						if (c == EOF) {
							return EOF;
						}
						if (c == '*') {
							if ((c = getNextChar()) == '/') {
								break;
							}
							putback(c);
						}
					}
					break;
				}
				if (c == '/') {
					// Comment
					for ( ; ; ) {
						c = getNextChar();
						if (c == EOF) {
							return EOF;
						}
						if (c == '\n') {
							break;
						}
					}
					break;
				}
				return '/';
				
			case '\"':
				return scanString('\"');

			case '\'':
				return scanString('\'');
			
			default:
				putback(c);
				if ((token = scanNumber()) != -1) {
					return token;
				}
				if ((token = scanSpecial()) != -1) {
					token = scanKeyword(buffer);
					return (token == T_IDENTIFIER) ? K_UNKNOWN : token;
				}
				if ((token = scanIdentifier()) != -1) {
					token = scanKeyword(buffer);
					return token;
				}
				return K_UNKNOWN;
		}
	}
	return EOF;
}