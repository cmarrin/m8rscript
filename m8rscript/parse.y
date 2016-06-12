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
#include "Program.h"
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
    m8r::StringId       string;
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

%right '?' ':'
%left O_LOR
%left O_LAND
%left '|'
%left '^'
%left '&'
%left O_EQ O_NE
%left '<' '>' O_LE O_GE
%left O_LSHIFT O_RSHIFT O_RSHIFTFILL
%left '+' '-'
%left '*' '/' '%'
%left UNARY

// We expect if..then..else to produce a shift/reduce conflict
// and empty statement and empty object literal produce another
// These both resolve correctly
%expect 2
%pure_parser
//%debug
//%verbose

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
    | array_literal
	| '(' expression ')'
	;
	
member_expression
	: primary_expression
	| function_expression
	| member_expression '[' expression ']'
	| member_expression '.' identifier { parser->emit(m8r::Op::DEREF); }
    | K_NEW member_expression arguments { parser->emitWithCount(m8r::Op::NEW, $3); }
    ;

new_expression
	: member_expression
	| K_NEW new_expression
	;

call_expression
	: member_expression arguments  { parser->emitWithCount(m8r::Op::CALL, $2); }
	| call_expression arguments  { parser->emitWithCount(m8r::Op::CALL, $2); }
    | call_expression '[' expression ']'
    | call_expression '.' identifier { parser->emit(m8r::Op::DEREF); }
	;

left_hand_side_expression
	: new_expression
	| call_expression
	;

mutation_expression
	: left_hand_side_expression O_INC { parser->emit(m8r::Op::POSTINC); }
	| left_hand_side_expression O_DEC { parser->emit(m8r::Op::POSTDEC); }
	| O_INC left_hand_side_expression { parser->emit(m8r::Op::PREINC); }
	| O_DEC left_hand_side_expression { parser->emit(m8r::Op::PREDEC); }
    ;

arguments
    : '(' ')' { $$ = 0; }
    | '(' argument_list ')' { $$ = $2; }
    ;
    
argument_list
	: expression { $$ = 1; }
	| argument_list ',' expression { $$++; }
	;

unary_operator
	: '+' { $$ = m8r::Op::UPLUS; }
	| '-' { $$ = m8r::Op::UMINUS; }
	| '~' { $$ = m8r::Op::UNEG; }
	| '!' { $$ = m8r::Op::UNOT; }
	;

arithmetic_expression
	: mutation_expression
    | left_hand_side_expression
    | unary_operator arithmetic_expression %prec UNARY { parser->emit($1); }
	| arithmetic_expression '*' arithmetic_expression { parser->emit(m8r::Op::MUL); }
	| arithmetic_expression '/' arithmetic_expression { parser->emit(m8r::Op::DIV); }
	| arithmetic_expression '%' arithmetic_expression { parser->emit(m8r::Op::MOD); }
	| arithmetic_expression '+' arithmetic_expression { parser->emit(m8r::Op::ADD); }
	| arithmetic_expression '-' arithmetic_expression { parser->emit(m8r::Op::SUB); }
	| arithmetic_expression O_LSHIFT arithmetic_expression { parser->emit(m8r::Op::SHL); }
	| arithmetic_expression O_RSHIFT arithmetic_expression { parser->emit(m8r::Op::SHR); }
	| arithmetic_expression O_RSHIFTFILL arithmetic_expression { parser->emit(m8r::Op::SAR); }
	| arithmetic_expression '<' arithmetic_expression { parser->emit(m8r::Op::LT); }
	| arithmetic_expression '>' arithmetic_expression { parser->emit(m8r::Op::GT); }
	| arithmetic_expression O_LE arithmetic_expression { parser->emit(m8r::Op::LE); }
	| arithmetic_expression O_GE arithmetic_expression { parser->emit(m8r::Op::GE); }
	| arithmetic_expression O_EQ arithmetic_expression { parser->emit(m8r::Op::EQ); }
	| arithmetic_expression O_NE arithmetic_expression { parser->emit(m8r::Op::NE); }
	| arithmetic_expression '&' arithmetic_expression { parser->emit(m8r::Op::AND); }
	| arithmetic_expression '^' arithmetic_expression { parser->emit(m8r::Op::XOR); }
	| arithmetic_expression '|' arithmetic_expression { parser->emit(m8r::Op::OR); }
	| arithmetic_expression O_LAND arithmetic_expression { parser->emit(m8r::Op::LAND); }
	| arithmetic_expression O_LOR arithmetic_expression { parser->emit(m8r::Op::LOR); }
	| arithmetic_expression '?' arithmetic_expression ':' arithmetic_expression
	;

assignment_expression
	: mutation_expression assignment_operator arithmetic_expression { parser->emit($2); }
	| mutation_expression assignment_operator assignment_expression { parser->emit($2); }
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
    | arithmetic_expression
    ;

variable_declaration_list:
		variable_declaration
    |	variable_declaration_list ',' variable_declaration
    ;

variable_declaration:
    	T_IDENTIFIER { parser->addVar($1); }
    |	T_IDENTIFIER { parser->addVar($1); parser->emit($1); } initializer
    ;

initializer:
    	'=' expression { parser->emit(m8r::Op::STOPOP); }
    ;

statement
	: ';'
    | compound_statement
	| K_VAR variable_declaration_list ';'
    | K_DELETE left_hand_side_expression ';'
    | call_expression ';'
    | mutation_expression { parser->emit(m8r::Op::POP); } ';'
    | assignment_expression  { parser->emit(m8r::Op::POP); } ';'
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

case_clauses_opt
    : case_clauses
	| /* empty */
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

for_loop_initializer
    : /* empty */
    | K_VAR T_IDENTIFIER { parser->addVar($2); parser->emit($2); } initializer
    | T_IDENTIFIER { parser->emit($1); } initializer
    ;

for_loop_initializers
    : for_loop_initializer
    | for_loop_initializers ',' for_loop_initializer
    ;

// For Loop code:
//
// emit <initializer>
// a = label()
// emit <conditional>
// addMatchedJump(JF, a)
// startDeferred()
// emit <incrementer>
// endDeferred()
// emit <statement>
// emitDeferred()
// matchJump() 
// 
iteration_statement
	: K_WHILE '(' iteration_start expression
        { parser->addMatchedJump(m8r::Op::JF, $3); }
      ')' statement
        { parser->matchJump($3); }
	| K_DO iteration_start statement K_WHILE '(' expression ')' ';'
	| K_FOR '(' for_loop_initializers ';' iteration_start expression 
        { parser->addMatchedJump(m8r::Op::JF, $5); parser->startDeferred(); }
      ';' expression
        { parser->emit(m8r::Op::POP); parser->endDeferred(); }
      ')' statement
        { parser->emitDeferred(); parser->matchJump($5); }
	;

jump_statement
	: K_CONTINUE ';'
	| K_BREAK ';'
	| K_RETURN ';' { parser->emitWithCount(m8r::Op::RET, 0); }
	| K_RETURN expression ';' { parser->emitWithCount(m8r::Op::RET, 1); }
	;

function_declaration : K_FUNCTION T_IDENTIFIER function { parser->addNamedFunction($3, $2); }

function_expression : K_FUNCTION function { parser->emit($2); } ;
    
formal_parameter_list
    :   /* empty */
    |   T_IDENTIFIER { parser->functionAddParam($1); }
    |	formal_parameter_list ',' T_IDENTIFIER { parser->functionAddParam($3); }
    ;
    
function
    :   '(' { parser->functionStart(); } 
        formal_parameter_list
            { parser->functionParamsEnd(); } 
        ')' '{' function_body '}'
            { parser->emit(m8r::Op::END); $$ = parser->functionEnd(); }
    ;
    
function_body
    : /* empty */
    | source_elements
    ;

array_literal
    : '[' ']'
    | '[' argument_list ']'
    ;

object_literal
    : '{' property_name_and_value_list '}'
    ;

property_name_and_value_list
    : property_assignment
    | property_name_and_value_list ',' property_assignment
    ;

property_assignment
    : 
    | property_name ':' expression
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
