/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
	  
    - Redistributions in binary form must reproduce the above copyright 
	  notice, this list of conditions and the following disclaimer in the 
	  documentation and/or other materials provided with the distribution.
	  
    - Neither the name of the <ORGANIZATION> nor the names of its 
	  contributors may be used to endorse or promote products derived from 
	  this software without specific prior written permission.
	  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

extern "C"{
    #include "ets_sys.h"
    #include "osapi.h"
    #include "gpio.h"
    #include "os_type.h"
    #include "uart.h"
}

#include "HardwareSerial.h"
#include "Esp.h"

int ICACHE_RAM_ATTR printf(const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
    int ret = ets_vprintf(ets_putc, format, arglist);
    va_end(arglist);
    return ret;
}

int ICACHE_RAM_ATTR vprintf(const char * format, va_list arg) {
    return ets_vprintf(ets_putc, format, arg);
}

extern "C" int ets_vsprintf(char *str, const char *format, va_list argptr);
int ICACHE_RAM_ATTR sprintf(char* buffer, const char* format, ...) {
    int ret;
    va_list arglist;
    va_start(arglist, format);
    ret = ets_vsprintf(buffer, format, arglist);
    va_end(arglist);
    return ret;
}

extern void runScript();

static const int ledPin = 2;

void setup()
{
	pinMode(ledPin, OUTPUT);

    Serial.begin(230400);
    delay(2000);
    
    Serial.print("Hello World!\n");

    runScript();
}

void loop()
{
    if (digitalRead(ledPin))
    {
        digitalWrite(ledPin, LOW);
        Serial.print("Blink Off!\n");
    }
    else
    {
        digitalWrite(ledPin, HIGH);
        Serial.print("Blink On!\n");
    }
    delay(1000);
}
