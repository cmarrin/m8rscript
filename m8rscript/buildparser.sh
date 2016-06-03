flex --outfile=lex.yy.cpp --header-file=lex.yy.h --yylineno --bison-bridge lexer.ll
bison -o parse.tab.cpp --defines=parse.tab.h  parse.y
