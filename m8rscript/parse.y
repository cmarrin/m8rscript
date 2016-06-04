/*
 _______________________________________________________________________
 |
 |
 |  Description:
 |	This file contains the bison rules for the m8rscript Parser
 |
 |   Author(s)		: Chris Marrin
 |
 _______________________________________________________________________
 */

%{
#include "Scanner.h"

#define YYERROR_VERBOSE

//#define YYSTYPE m8r::Scanner::TokenValue

inline void yyerror(m8r::Scanner* scanner, const char* s)
{
    scanner->printError(s);
}

int yylex(YYSTYPE* token, m8r::Scanner* scanner)
{
    uint8_t t = scanner->getToken(token);
    return (t == C_EOF) ? 0 : t;
}

%}

/* keywords go from 1-32, where the control codes and space go */

%token K_FUNCTION		1
%token K_NEW			2
%token K_DELETE			3
%token K_VAR			4
%token K_DO				10
%token K_WHILE			11
%token K_FOR			12
%token K_IF				13
%token K_ELSE			14
%token K_SWITCH			15
%token K_CASE			16
%token K_DEFAULT		17
%token K_BREAK			18
%token K_CONTINUE		19
%token K_RETURN			20

%token K_UNKNOWN		21
%token K_COMMENT		22

/* 33-47 are special chars and can't be used */

/* 48-57 are the digits and can be used */
%token T_FLOAT			48
%token T_IDENTIFIER		49
%token T_STRING			50
%token T_INTEGER		51

/* 58-64 are special chars and can't be used */

/* 65-90 are letters and can be used */

%token O_RSHIFTEQ		65
%token O_RSHIFTFILLEQ	66
%token O_LSHIFTEQ		67
%token O_ADDEQ			68
%token O_SUBEQ			69
%token O_MULEQ			70
%token O_DIVEQ			71
%token O_MODEQ			72
%token O_ANDEQ			73
%token O_XOREQ			74
%token O_OREQ			75
%token O_RSHIFT			76
%token O_RSHIFTFILL		77
%token O_LSHIFT			78
%token O_INC			79
%token O_DEC			80
%token O_LAND			81
%token O_LOR			82
%token O_LE				83
%token O_GE				84
%token O_EQ				85
%token O_NE				86

%token E_ERROR			191

%token C_EOF			255 "end of file"

%union {
    const char*         string;
    float				number;
    uint32_t            integer;
	m8r::Atom           atom;
};

%type <string>		T_STRING
%type <atom>		T_IDENTIFIER
%type <integer>		T_INTEGER
%type <number>		T_FLOAT

/*  we expect if..then..else to produce a shift/reduce conflict */
%expect 1
%pure_parser
//%debug

%lex-param { m8r::Scanner* scanner }
%parse-param { m8r::Scanner* scanner }

%start program
%%

program
    : source_elements
    ;

source_elements
    : source_element
    | source_elements source_element
    ;

source_element
    : statement
    | function_declaration
    ;
	
primary_expression
	: identifier
    | T_FLOAT { scanner->emit($1); }
	| T_INTEGER { scanner->emit($1); }
    | T_STRING { scanner->emit($1); }
    | object_literal
	| '(' expression ')'
	;
	
member_expression
	: primary_expression
	| function_expression
	| member_expression '[' expression ']'
	| member_expression '.' identifier { scanner->emit(m8r::Scanner::OpcodeType::Deref); }
    | K_NEW member_expression arguments
    ;

new_expression
	: member_expression
	| K_NEW new_expression
	;

call_expression
	: member_expression arguments
	| call_expression arguments
    | call_expression '[' expression ']'
    | call_expression '.' identifier { scanner->emit(m8r::Scanner::OpcodeType::Deref); }
	;

left_hand_side_expression
	: new_expression
	| call_expression
	;

postfix_expression
	: left_hand_side_expression
	| left_hand_side_expression O_INC
	| left_hand_side_expression O_DEC
    ;

arguments
    : '(' ')'
    | '(' argument_list ')'
    ;
    
argument_list
	: assignment_expression
	| argument_list ',' assignment_expression
	;

unary_expression
	: postfix_expression
	| unary_operator unary_expression
	;

unary_operator
	: '+'
	| '-'
	| '~'
	| '!'
	| K_DELETE
	| O_INC
	| O_DEC
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression
	| multiplicative_expression '/' unary_expression
	| multiplicative_expression '%' unary_expression
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
	;

shift_expression
	: additive_expression
	| shift_expression O_LSHIFT additive_expression
	| shift_expression O_RSHIFT additive_expression
	| shift_expression O_RSHIFTFILL additive_expression
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression O_LE shift_expression
	| relational_expression O_GE shift_expression
	;

equality_expression
	: relational_expression
	| equality_expression O_EQ relational_expression
	| equality_expression O_NE relational_expression
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression O_LAND inclusive_or_expression
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression O_LOR logical_and_expression
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator
	: '='
	| O_MULEQ
	| O_DIVEQ
	| O_MODEQ
	| O_ADDEQ
	| O_SUBEQ
	| O_LSHIFTEQ
	| O_RSHIFTEQ
	| O_RSHIFTFILLEQ
	| O_ANDEQ
	| O_XOREQ
	| O_OREQ
	;

expression
	: assignment_expression
	| expression ',' assignment_expression
	;

declaration_statement
	: K_VAR variable_declaration_list ';'
	;
	
variable_declaration_list:
		variable_declaration
    |	variable_declaration_list ',' variable_declaration
    ;

variable_declaration:
    	identifier
    |	identifier initializer
    ;

initializer:
    	'=' assignment_expression
    ;

statement
	: compound_statement
	| declaration_statement
	| expression_statement
	| selection_statement
	| switch_statement
	| iteration_statement
	| jump_statement
	;

compound_statement
	: '{' '}'
	| '{' statement_list '}'
	;

statement_list
	: statement
	| statement_list statement
	;

expression_statement
	: ';'
	| expression ';'
	;

selection_statement
	: K_IF '(' expression ')' statement
	| K_IF '(' expression ')' statement K_ELSE statement
	;
	
switch_statement:
		K_SWITCH '(' expression ')' case_block
	;

case_block:
		'{' case_clauses_opt '}'
	|	'{' case_clauses_opt default_clause case_clauses_opt '}'
	;

case_clauses_opt:
		case_clauses
	|	/* empty */
	;

case_clauses:
		case_clause
	|	case_clauses case_clause
	;

case_clause:
		K_CASE expression ':' statement
	;

default_clause:
		K_DEFAULT ':' statement
	;

iteration_statement
	: K_WHILE '(' expression ')' statement
	| K_DO statement K_WHILE '(' expression ')' ';'
	| K_FOR '(' expression_statement expression_statement expression ')' statement
	;

jump_statement
	: K_CONTINUE ';'
	| K_BREAK ';'
	| K_RETURN ';'
	| K_RETURN expression ';'
	;

function_declaration
	: K_FUNCTION identifier '(' ')' '{' function_body '}'
	| K_FUNCTION identifier '(' formal_parameter_list ')' '{' function_body '}'
	;

function_expression
    : K_FUNCTION '(' ')' '{' function_body '}'
    | K_FUNCTION '(' formal_parameter_list ')' '{' function_body '}'
    ;
    
formal_parameter_list:
		identifier
    |	formal_parameter_list ',' identifier
    ;
    
function_body
    : /* empty */
    | source_elements
    ;

object_literal
    : '[' ']'
	| '[' property_name_and_value_list ']'
    ;

property_name_and_value_list
    : property_assignment
    | property_name_and_value_list ',' property_assignment
    ;

property_assignment
    : property_name ':' assignment_expression
    ;
    
property_name
    : identifier
    | T_STRING { scanner->emit($1); }
    | T_FLOAT { scanner->emit($1); }
    | T_INTEGER { scanner->emit($1); }
    ;

identifier
    : T_IDENTIFIER { scanner->emit($1); }
    ;

%%
