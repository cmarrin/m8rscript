#include <SmingCore/SmingCore.h>

#include "Parser.h"
#include "Stream.h"
#include "CodePrinter.h"
#include "SystemInterface.h"

void __assert_func(const char *file, int line, const char *func, const char *what)
{
    SYSTEM_ERROR("Assertion failed: (%s), function %s, file %s, line %d", what, func, file, line);
    while (1) ;
}

Timer procTimer;
bool state = true;

void blink()
{
	digitalWrite(2, state);
	state = !state;
}

static m8r::String insertCR(const char* s)
{
    m8r::String ss;
    while(*s != '\0') {
        if (*s == '\n') {
            ss += '\r';
        }
        ss += *s++;
    }
    return ss;
}

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void print(const char* s) const override
    {
        m8r::String ss = insertCR(s);
        Serial.print(ss.c_str());
    }
};

void init() {
    pinMode(2, OUTPUT);

    const char* filename = "simple.m8r";
    
    MySystemInterface systemInterface;
    systemInterface.print("\n*** m8rscript v0.1\n\n");
    
//    systemInterface.print("Opening '");
//    systemInterface.print(filename);
//    systemInterface.print("'\n");
//    m8r::FileStream istream(filename);
//    if (!istream.loaded()) {
//        systemInterface.print("File not found, exiting\n");
//        abort();
//    }

    m8r::StringStream istream("var a = 4 + 7; Serial.print(\"It's Happening!!!\" + a + \"\n\");");
    systemInterface.print("Parsing...\n");
    m8r::Parser parser(&istream, &systemInterface);
    
    systemInterface.print("Finished. ");
    systemInterface.print(m8r::Value::toString(parser.nerrors()).c_str());
    systemInterface.print(" error");
    systemInterface.print((parser.nerrors() == 1) ? "" : "s");
    systemInterface.print("\n\n");

    if (!parser.nerrors()) {
        systemInterface.print("\n***** Start of Program Output *****\n\n");
        m8r::ExecutionUnit eu(&systemInterface);
        eu.run(parser.program());
        systemInterface.print("\n***** End of Program Output *****\n\n");
    }

	procTimer.initializeMs(1000, blink).start();
}
