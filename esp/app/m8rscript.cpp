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
    #include "Esp.h"
    #include "osapi.h"
    #include "gpio.h"
    #include "os_type.h"
    #include "user_interface.h"
    //#include "uart.h"
}

//#include "HardwareSerial.h"

extern void runScript();

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

void some_timerfunc(void *arg)
{
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        gpio_output_set(0, BIT2, BIT2, 0);
    } else {
        gpio_output_set(BIT2, 0, BIT2, 0);
    }
}

static uint32_t CounterTime = 400;

static void ICACHE_FLASH_ATTR user_procTask(os_event_t *events)
{
    static uint32_t counter = CounterTime;
    static int holdoff = 10;
    
    if (--counter == 0) {
        counter = CounterTime;
        some_timerfunc(0);
        if (holdoff-- == 0) {
            struct softap_config config;
            if (wifi_softap_get_config_default(&config)) {
                os_printf("wifi_softap_get_config:\n");
                os_printf("    ssid = %s\n", config.ssid);
                os_printf("    password = %s\n", config.password);
                os_printf("    channel = %d\n", config.channel);
            } else {
                os_printf("wifi_softap_get_config FAILED\n");
            }
            
            runScript();
        }
    }
    os_delay_us(1000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

extern "C" void ICACHE_FLASH_ATTR user_init()
{
    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );

    // init gpio sussytem
    gpio_init();

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    gpio_output_set(0, BIT2, BIT2, 0);

    //os_timer_setfn((os_timer_t*) &some_timer, (os_timer_func_t *)some_timerfunc, NULL);
    //os_timer_arm((os_timer_t*) &some_timer, 1000, 1);

    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
}

//void ICACHE_FLASH_ATTR user_init()
//{
//    //Serial.begin(115200);
//    //delay(2000);
//    
//    Serial.print("Hello World!\n");
//
//    runScript();
//}

//void setup()
//{
//	pinMode(ledPin, OUTPUT);
//
//    Serial.begin(230400);
//    delay(2000);
//    
//    Serial.print("Hello World!\n");
//
//    runScript();
//}
//
//void loop()
//{
//    if (digitalRead(ledPin))
//    {
//        digitalWrite(ledPin, LOW);
//        Serial.print("Blink Off!\n");
//    }
//    else
//    {
//        digitalWrite(ledPin, HIGH);
//        Serial.print("Blink On!\n");
//    }
//    delay(1000);
//}
