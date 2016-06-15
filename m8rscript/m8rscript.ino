#include <FS.h>

#include "Parser.h"

void print(const char* s) { }

void setup() {
    // put your setup code here, to run once:
    File istream;
    m8r::Parser parser(&istream, print);
    m8r::ExecutionUnit eu;
    eu.generateCodeString(parser.program()).c_str();
    eu.run(parser.program(), print);
}

void loop() {
  // put your main code here, to run repeatedly:

}
