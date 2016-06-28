#include "Parser.h"
#include "Stream.h"
#include "Printer.h"

class MyPrinter : public m8r::Printer
{
public:
    virtual void print(const char*) const override { }
};

extern "C" void user_init()
{
    MyPrinter printer;
    // put your setup code here, to run once:
    m8r::FileStream istream("");
    m8r::Parser parser(&istream, &printer);
    m8r::ExecutionUnit eu(&printer);
    eu.generateCodeString(parser.program()).c_str();
    eu.run(parser.program());

    //m8r::Program program;
    //eu.run(&program, print);    
}
