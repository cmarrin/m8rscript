echo "===== Building yyparse version ====="
touch ../m8rscript/Parser.h
make USE_PARSE_ENGINE=0
echo "===== Building ParseEngine version ====="
touch ../m8rscript/Parser.h
make USE_PARSE_ENGINE=1
echo "===== Done ====="
