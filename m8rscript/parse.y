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
#include "Atom.h"
#include "Function.h"
#include "Parser.h"

#define YYERROR_VERBOSE

inline void yyerror(m8r::Parser* parser, const char* s)
{
    parser->printError(s);
}

int yylex(YYSTYPE* token, m8r::Parser* parser)
{
    uint8_t t = parser->getToken(token);
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
    m8r::Op             op;
    m8r::Label          label;
    m8r::Function*      function;
    const char*         string;
    float				number;
    uint32_t            integer;
	m8r::Atom           atom;
    uint32_t            argcount;
};

%type <string>		T_STRING
%type <atom>		T_IDENTIFIER
%type <integer>		T_INTEGER
%type <number>		T_FLOAT
%type <argcount>    argument_list arguments
%type <op>          assignment_operator unary_operator
%type <label>       iteration_start
%type <function>    function

/*  we expect if..then..else to produce a shift/reduce conflict */
%expect 1
%pure_parser
//%debug

%lex-param { m8r::Parser* parser }
%parse-param { m8r::Parser* parser }

%start program
%%

program
    : source_elements { parser->programEnd(); }
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
    | T_FLOAT { parser->emit($1); }
	| T_INTEGER { parser->emit($1); }
    | T_STRING { parser->emit($1); }
    | object_literal
	| '(' expression ')'
	;
	
member_expression
	: primary_expression
	| function_expression
	| member_expression '[' expression ']'
	| member_expression '.' identifier { parser->emit(m8r::Op::DEREF); }
    | K_NEW member_expression arguments { parser->emitCallOrNew(false, $3); }
    ;

new_expression
	: member_expression
	| K_NEW new_expression
	;

call_expression
	: member_expression arguments  { parser->emitCallOrNew(true, $2); }
	| call_expression arguments  { parser->emitCallOrNew(true, $2); }
    | call_expression '[' expression ']'
    | call_expression '.' identifier { parser->emit(m8r::Op::DEREF); }
	;

left_hand_side_expression
	: new_expression
	| call_expression
	;

postfix_expression
	: left_hand_side_expression
	| left_hand_side_expression O_INC { parser->emit(m8r::Op::POSTINC); }
	| left_hand_side_expression O_DEC { parser->emit(m8r::Op::POSTDEC); }
    ;

arguments
    : '(' ')' { $$ = 0; }
    | '(' argument_list ')' { $$ = $2; }
    ;
    
argument_list
	: assignment_expression { $$ = 1; }
	| argument_list ',' assignment_expression { $$++; }
	;

unary_expression
	: postfix_expression
	| unary_operator unary_expression { parser->emit($1); }
	;

unary_operator
	: '+' { $$ = m8r::Op::UPLUS; }
	| '-' { $$ = m8r::Op::UMINUS; }
	| '~' { $$ = m8r::Op::UNEG; }
	| '!' { $$ = m8r::Op::UNOT; }
	| K_DELETE { $$ = m8r::Op::DEL; }
	| O_INC { $$ = m8r::Op::PREINC; }
	| O_DEC { $$ = m8r::Op::PREDEC; }
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression { parser->emit(m8r::Op::MUL); }
	| multiplicative_expression '/' unary_expression { parser->emit(m8r::Op::DIV); }
	| multiplicative_expression '%' unary_expression { parser->emit(m8r::Op::MOD); }
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression { parser->emit(m8r::Op::ADD); }
	| additive_expression '-' multiplicative_expression { parser->emit(m8r::Op::SUB); }
	;

shift_expression
	: additive_expression
	| shift_expression O_LSHIFT additive_expression { parser->emit(m8r::Op::SHL); }
	| shift_expression O_RSHIFT additive_expression { parser->emit(m8r::Op::SHR); }
	| shift_expression O_RSHIFTFILL additive_expression { parser->emit(m8r::Op::SAR); }
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression { parser->emit(m8r::Op::LT); }
	| relational_expression '>' shift_expression { parser->emit(m8r::Op::GT); }
	| relational_expression O_LE shift_expression { parser->emit(m8r::Op::LE); }
	| relational_expression O_GE shift_expression { parser->emit(m8r::Op::GE); }
	;

equality_expression
	: relational_expression
	| equality_expression O_EQ relational_expression { parser->emit(m8r::Op::EQ); }
	| equality_expression O_NE relational_expression { parser->emit(m8r::Op::NE); }
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression { parser->emit(m8r::Op::AND); }
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression { parser->emit(m8r::Op::XOR); }
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression { parser->emit(m8r::Op::OR); }
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression O_LAND inclusive_or_expression { parser->emit(m8r::Op::LAND); }
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression O_LOR logical_and_expression { parser->emit(m8r::Op::LOR); }
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression { parser->emit($2); }
	;

assignment_operator
	: '=' { $$ = m8r::Op::STO; }
	| O_MULEQ { $$ = m8r::Op::STOMUL; }
	| O_DIVEQ { $$ = m8r::Op::STODIV; }
	| O_MODEQ { $$ = m8r::Op::STOMOD; }
	| O_ADDEQ { $$ = m8r::Op::STOADD; }
	| O_SUBEQ { $$ = m8r::Op::STOSUB; }
	| O_LSHIFTEQ { $$ = m8r::Op::STOSHL; }
	| O_RSHIFTEQ { $$ = m8r::Op::STOSHR; }
	| O_RSHIFTFILLEQ { $$ = m8r::Op::STOSAR; }
	| O_ANDEQ { $$ = m8r::Op::STOAND; }
	| O_XOREQ { $$ = m8r::Op::STOXOR; }
	| O_OREQ { $$ = m8r::Op::STOOR; }
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
    	identifier { parser->emit(m8r::Op::NEWID); }
    |	identifier { parser->emit(m8r::Op::NEWID); } initializer
    ;

initializer:
    	'=' assignment_expression { parser->emit(m8r::Op::STO); }
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

iteration_start: { $$ = parser->label(); } ;

iteration_statement
	: K_WHILE iteration_start '(' { parser->loopStart(false, $2); } expression ')' statement { parser->loopEnd($2); }
	| K_DO iteration_start statement K_WHILE '(' expression ')' ';'
	| K_FOR '(' expression_statement iteration_start expression_statement expression ')' statement
	;

jump_statement
	: K_CONTINUE ';'
	| K_BREAK ';'
	| K_RETURN ';'
	| K_RETURN expression ';'
	;

function_declaration : K_FUNCTION T_IDENTIFIER function { parser->addNamedFunction($3, $2); }

function_expression : K_FUNCTION function { parser->addObject($2); } ;
    
formal_parameter_list
    :   /* empty */
    |   T_IDENTIFIER { parser->functionAddParam($1); }
    |	formal_parameter_list ',' T_IDENTIFIER
    ;
    
function
    :   '(' { parser->functionStart(); } 
        formal_parameter_list ')' '{' function_body '}' { parser->emit(m8r::Op::END); $$ = parser->functionEnd(); }
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
    | T_STRING { parser->emit($1); }
    | T_FLOAT { parser->emit($1); }
    | T_INTEGER { parser->emit($1); }
    ;

identifier
    : T_IDENTIFIER { parser->emit($1); }
    ;

%%
