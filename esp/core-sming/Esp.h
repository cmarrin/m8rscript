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

#define USE_SMING

#include <SmingCore/SmingCore.h>
#undef min
#undef max

#ifdef __cplusplus
extern "C" {
#endif

#include "user_config.h"
#include "osapi.h"

#include <stdint.h>
#include <stdarg.h>
#include <ets_sys.h>
#include <esp_systemapi.h>

void initializeSystem();
uint64_t currentMicroseconds();
static inline int readSerialChar() { return 0; /*Serial.read();*/ }

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

void ets_delay_us(uint32_t);

#ifdef __cplusplus
} // extern "C"
#endif
