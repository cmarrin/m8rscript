#include "Parser.h"
#include "Stream.h"
#include "Printer.h"

class MyPrinter : public m8r::Printer
{
public:
    virtual void print(const char*) const override { }
};

extern "C" void setup()
{
    // put your setup code here, to run once:
    m8r::FileStream istream("");
    m8r::Parser parser(&istream, MyPrinter);
    m8r::ExecutionUnit eu(MyPrinter);
    eu.generateCodeString(parser.program()).c_str();
    eu.run(parser.program());

    //m8r::Program program;
    //eu.run(&program, print);
}

extern "C" void loop() { }

//extern "C" void user_init() { }
