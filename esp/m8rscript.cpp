#include "Parser.h"
#include "Stream.h"

void print(const char* s) { }

extern "C" void setup()
{
    // put your setup code here, to run once:
    m8r::FileStream istream("");
    m8r::Parser parser(&istream, ::print);
    m8r::ExecutionUnit eu(::print);
    eu.generateCodeString(parser.program().get()).c_str();
    eu.run(parser.program().get());

    //m8r::Program program;
    //eu.run(&program, print);
}

extern "C" void loop() { }

//extern "C" void user_init() { }