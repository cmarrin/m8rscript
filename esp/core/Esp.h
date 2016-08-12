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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "user_config.h"
#include "umm_malloc.h"
#include "osapi.h"

#include <stdint.h>
#include <stdarg.h>
#include <ets_sys.h>

#define HIGH 0x1
#define LOW  0x0

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

typedef void (*int_handler_t)(void*);

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
#define ICACHE_FLASH_ATTR   __attribute__((section(".irom0.text")))
#define ICACHE_RAM_ATTR     __attribute__((section(".iram.text")))
#define ICACHE_RODATA_ATTR  __attribute__((section(".irom.text")))
#define ICACHE_STORE_ATTR   __attribute__((aligned(4)))

static inline uint8_t ICACHE_FLASH_ATTR read_rom_uint8(const uint8_t* addr)
{
    uint32_t bytes;
    bytes = *(uint32_t*)((uint32_t)addr & ~3);
    return ((uint8_t*)&bytes)[(uint32_t)addr & 3];
}

#define panic() __assert_func(__FILE__, __LINE__, __func__, "panic")

void yield(void);
void optimistic_yield(uint32_t interval_us);
uint64_t millis(void);
uint64_t micros(void);
void delay(uint32_t ms);
void delayMicroseconds(uint16_t us);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogReference(uint8_t mode);
void analogWrite(uint8_t pin, int val);
void analogWriteFreq(uint32_t freq);
void analogWriteRange(uint32_t range);

//void *ets_memcpy(void *dest, const void *src, size_t n);
//void *ets_memset(void *s, int c, size_t n);
//void ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);
//void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *fn, void *parg);
//void ets_timer_disarm(ETSTimer *a);
//int atoi(const char *nptr);
//int ets_strncmp(const char *s1, const char *s2, int len);
//int ets_strcmp(const char *s1, const char *s2);
//int ets_strlen(const char *s);
//char *ets_strcpy(char *dest, const char *src);
//char *ets_strncpy(char *dest, const char *src, size_t n);
//char *ets_strstr(const char *haystack, const char *needle);
//int ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
//int os_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
//int ets_printf(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
//void ets_install_putc1(void* routine);
//void uart_div_modify(int no, int freq);
//void ets_isr_mask(int intr);
//void ets_isr_unmask(int intr);
//void ets_isr_attach(int intr, int_handler_t handler, void *arg);
//void ets_intr_lock();
//void ets_intr_unlock();
int os_printf_plus(const char * format, ...);
//int ets_vsnprintf(char * s, size_t n, const char * format, va_list arg)  __attribute__ ((format (printf, 3, 0)));
//int ets_vprintf(int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
//int ets_vsprintf(char *str, const char *format, va_list argptr);
//int ets_putc(int);
//bool ets_task(ETSTask task, uint8 prio, ETSEvent *queue, uint8 qlen);
//bool ets_post(uint8 prio, ETSSignal sig, ETSParam par);
void ets_delay_us(uint32_t);

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
