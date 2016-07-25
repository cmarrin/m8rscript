/*
 Esp.h - ESP8266-specific APIs
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ESP_H
#define ESP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//GPIO FUNCTIONS
#define INPUT             0x00
#define INPUT_PULLUP      0x02
#define INPUT_PULLDOWN_16 0x04 // PULLDOWN only possible for pin16
#define OUTPUT            0x01
#define OUTPUT_OPEN_DRAIN 0x03
#define WAKEUP_PULLUP     0x05
#define WAKEUP_PULLDOWN   0x07
#define SPECIAL           0xF8 //defaults to the usable BUSes uart0rx/tx uart1tx and hspi
#define FUNCTION_0        0x08
#define FUNCTION_1        0x18
#define FUNCTION_2        0x28
#define FUNCTION_3        0x38
#define FUNCTION_4        0x48

//Interrupt Modes
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D

#define timer1_read()           (T1V)
#define timer1_enabled()        ((T1C & (1 << TCTE)) != 0)
#define timer1_interrupted()    ((T1C & (1 << TCIS)) != 0)

typedef void(*timercallback)(void);

void timer1_isr_init(void);
void timer1_enable(uint8_t divider, uint8_t int_type, uint8_t reload);
void timer1_disable(void);
void timer1_attachInterrupt(timercallback userFunc);
void timer1_detachInterrupt(void);
void timer1_write(uint32_t ticks); //maximum ticks 8388607

extern void abort();

#ifndef __STRINGIFY
#define __STRINGIFY(a) #a
#endif

void __assert_func(const char *file, int line, const char *func, const char *what);
#define assert(expr) { if (expr) __assert_func(__FILE__, __LINE__, __func__, __STRINGIFY(expr)); }
#define panic() __assert_func(__FILE__, __LINE__, __func__, "panic")

void setup(void);
void loop(void);

void yield(void);
void optimistic_yield(uint32_t interval_us);
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long);
void delayMicroseconds(unsigned int us);

// these low level routines provide a replacement for SREG interrupt save that AVR uses
// but are esp8266 specific. A normal use pattern is like
//
//{
//    uint32_t savedPS = xt_rsil(1); // this routine will allow level 2 and above
//    // do work here
//    xt_wsr_ps(savedPS); // restore the state
//}
//
// level (0-15), interrupts of the given level and above will be active
// level 15 will disable ALL interrupts,
// level 0 will enable ALL interrupts,
//
#define xt_rsil(level) (__extension__({uint32_t state; __asm__ __volatile__("rsil %0," __STRINGIFY(level) : "=a" (state)); state;}))
#define xt_wsr_ps(state)  __asm__ __volatile__("wsr %0,ps; isync" :: "a" (state) : "memory")

#define interrupts() xt_rsil(0)
#define noInterrupts() xt_rsil(15)

#ifdef __cplusplus
} // extern "C"
#endif

#endif //ESP_H
