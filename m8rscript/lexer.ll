D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+

%{
#include "parse.tab.h"

#define register

%}

%option reentrant

%%
"/*" {
	int c1 = 0;
	int c2 = input();
	for ( ; ; ) {
		if (c2 == EOF) {
			break;
		}
		if (c1 == '*' && c2 == '/') {
			break;
		}
		c1 = c2;
		c2 = input();
	}
}

"break"			{ return(K_BREAK); }
"case"			{ return(K_CASE); }
"continue"		{ return(K_CONTINUE); }
"default"		{ return(K_DEFAULT); }
"delete"		{ return(K_DELETE); }
"do"			{ return(K_DO); }
"else"			{ return(K_ELSE); }
"for"			{ return(K_FOR); }
"function"		{ return(K_FUNCTION); }
"if"			{ return(K_IF); }
"new"			{ return(K_NEW); }
"return"		{ return(K_RETURN); }
"switch"		{ return(K_SWITCH); }
"var"			{ return(K_VAR); }
"while"			{ return(K_WHILE); }

{L}({L}|{D})*	{ return(T_IDENTIFIER); }

0[xX]{H}+		{ return(T_INTEGER); }
{D}+			{ return(T_INTEGER); }
{D}+{E}			{ return(T_FLOAT); }
{D}*"."{D}+({E})?	{ return(T_FLOAT); }
{D}+"."{D}*({E})?	{ return(T_FLOAT); }

\"(\\.|[^\\"])*\"	{ return(T_STRING); }
\'(\\.|[^\\'])*\'	{ return(T_STRING); }

">"				{ return('>'); }
">>"			{ return(O_RSHIFT); }
">="			{ return(O_GE); }
">>="			{ return(O_RSHIFTEQ); }
">>>"			{ return(O_RSHIFTFILL); }
">>>="			{ return(O_RSHIFTFILLEQ); }
"<"				{ return('<'); }
"<<"			{ return(O_LSHIFT); }
"<="			{ return(O_LE); }
"<<="			{ return(O_LSHIFTEQ); }
"+"				{ return('+'); }
"++"			{ return(O_INC); }
"+="			{ return(O_ADDEQ); }
"-"				{ return('-'); }
"--"			{ return(O_DEC); }
"-="			{ return(O_SUBEQ); }
"*"				{ return('*'); }
"*="			{ return(O_MULEQ); }
"/"				{ return('/'); }
"/="			{ return(O_DIVEQ); }
"%"				{ return('%'); }
"%="			{ return(O_MODEQ); }
"&"				{ return('&'); }
"&&"			{ return(O_LAND); }
"&="			{ return(O_ANDEQ); }
"^"				{ return('^'); }
"^="			{ return(O_XOREQ); }
"|"				{ return('|'); }
"||"			{ return(O_LOR); }
"|="			{ return(O_OREQ); }
"=="			{ return(O_EQ); }
"!"				{ return('!'); }
"!="			{ return(O_NE); }
";"				{ return(';'); }
"{"				{ return('{'); }
"}"				{ return('}'); }
","				{ return(','); }
":"				{ return(':'); }
"="				{ return('='); }
"("				{ return('('); }
")"				{ return(')'); }
"["				{ return('['); }
"]"				{ return(']'); }
"."				{ return('.'); }
"~"				{ return('~'); }
"?"				{ return('?'); }

[ \t\v\n\f]		{ }
.				{ /* ignore bad characters */ }

%%
