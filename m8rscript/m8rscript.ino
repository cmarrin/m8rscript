#include <FS.h>

#include "Parser.h"

void setup() {
    // put your setup code here, to run once:
    File istream;
    m8r::Parser parser(&istream);
    m8r::ExecutionUnit eu;
    eu.generateCodeString(parser.program()).c_str();
    eu.run(parser.program());
}

void loop() {
  // put your main code here, to run repeatedly:

}
