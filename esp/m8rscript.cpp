#include "Parser.h"
#include "Stream.h"
#include "CodePrinter.h"
#include "SystemInterface.h"

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void print(const char*) const override { }
};

extern "C" void user_init()
{
    MySystemInterface systemInterface;
    // put your setup code here, to run once:
    m8r::FileStream istream("");
    m8r::Parser parser(&istream, &systemInterface);
    m8r::ExecutionUnit eu(&systemInterface);
    eu.run(parser.program());

    //m8r::Program program(&printer);
    //eu.run(&program);
}
