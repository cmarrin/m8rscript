#include "Parser.h"
#include "Stream.h"
#include "CodePrinter.h"
#include "SystemInterface.h"

#include "Arduino.h"

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void print(const char*) const override { }
};

void setup() {
    pinMode(2, OUTPUT);
    Serial.begin(9600);

    MySystemInterface systemInterface;
    // m8r::FileStream istream("");
    // m8r::Parser parser(&istream, &systemInterface);
    m8r::ExecutionUnit eu(&systemInterface);
    // eu.run(parser.program());

    m8r::Program program(&systemInterface);
    eu.run(&program);
}

void loop() {
    Serial.println("Hello!!!");
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(500);              // wait for a second
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
    delay(500);              // wait for a second
}
