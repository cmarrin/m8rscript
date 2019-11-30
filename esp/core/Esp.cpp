/*
 Esp.cpp - ESP8266-specific APIs
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

#include "Esp.h"

#include "Mallocator.h"

#ifndef NDEBUG
#include <gdbstub.h>
#endif

extern "C" {
#include <cxxabi.h>
#include <user_interface.h>

size_t strspn(const char *str1, const char *str2) { return 0; }
size_t strcspn ( const char * str1, const char * str2 ) { return 0; }

// Needed by lwip
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
extern "C" int isxdigit(int c)        { return isHex(c) || isdigit(c); }

extern const uint32_t __attribute__((section(".ver_number"))) core_version = 0;

uint32 user_rf_cal_sector_set(void) {
    extern SpiFlashChip* flashchip;
    SpiFlashChip *flash = (SpiFlashChip*)(&flashchip + 4);
    // We know that sector size in 4096
    //uint32_t sec_num = flash->chip_size / flash->sector_size;
    uint32_t sec_num = flash->chip_size >> 12;
    return sec_num - 5;
}

} // extern "C"

char *strchr(const char *s, int c)
{
    while (*s != (char)c)
        if (!*s++)
            return 0;
    return (char *)s;
}

void *memchr(const void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char*)s;
    while( n-- )
        if( *p != (unsigned char)c )
            p++;
        else
            return p;
    return 0;
}

void* ROMmemcpy(void* dst, m8r::ROMString src, size_t len)
{
    uint8_t* d = (uint8_t*) dst;
    while (len--) {
        *d++ = readRomByte(src);
        src += 1;
    }
    return dst;
}

char* ROMCopyString(char* dst, m8r::ROMString src)
{
    char c;
    while ((c = (char) readRomByte(src))) {
        *dst++ = c;
        src += 1;
    }
    *dst = '\0';
    return dst;
}

size_t ROMstrlen(m8r::ROMString s)
{
    m8r::ROMString p = s;
    for ( ; readRomByte(p) != '\0'; p += 1) ;
    return (size_t) (p.value() - s.value());
}

m8r::ROMString ROMstrstr(m8r::ROMString s1, const char* s2)
{
    int i, j;

    if (!s1.valid() || s2 == nullptr) {
        return m8r::ROMString();
    }

    for( i = 0; ; i++) {
        char c1 = readRomByte(s1 + i);
        if (c1 == '\0') {
            return m8r::ROMString();
        }
        
        char c2 = *s2;
        if (c1 == c2) {
            for (j = i; ; j++) {
                c2 = *(s2 + (j - i));
                if (c2 == '\0') {
                    return m8r::ROMString(s1 + i);
                }
                c1 = readRomByte(s1 + j);
                if (c1 != c2) {
                    break;
                }
            }
        }
    }
}

int ROMstrcmp(m8r::ROMString s1, const char* s2)
{
    uint8_t c1;
    uint8_t c2;
    for (int32_t i = 0; ; i++) {
        c1 = readRomByte(s1+i);
        c2 = s2[i];
        if (c1 != c2) {
            break;
        }
        if (c1 == '\0') {
            return 0;
        }
    }
    return c1 - c2;
}

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

void do_global_ctors(void) {
    void (**p)(void) = &__init_array_end;
    while (p != &__init_array_start)
        (*--p)();
}

void m8r::heapInfo(void*& start, uint32_t& size)
{
    start = &_heap_start;
    size = _heap_end - _heap_start;
}

extern "C" {

void* RAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
	return m8r::Mallocator::shared()->allocate<char>(m8r::MemoryType::Fixed, size).get();
}

void RAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
    m8r::Mallocator::shared()->deallocate<char>(m8r::MemoryType::Fixed, m8r::Mad<char>(reinterpret_cast<char*>(ptr)), 0);
}

void* RAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
	void* m = pvPortMalloc(size, file, line);
    memset(m, 0, size);
    return m;
}

size_t xPortGetFreeHeapSize(void)
{
	return 0;
}

size_t RAM_ATTR xPortWantedSizeAlign(size_t size)
{
    return (size + 3) & ~((size_t) 3);
}

#ifndef NDEBUG
extern void gdb_do_break();
#else
#define gdb_do_break()
#endif

int atexit(void (*function)(void)) { }

[[noreturn]] void abort()
{
    do {
        *((int*)0) = 0;
    } while(true);
}

[[noreturn]] void __assert_func(const char *file, int line, const char *func, const char *what) {
    os_printf_plus(ROMSTR("ASSERT:(%s) at %s:%d\n").value(), what, func, line);
    gdb_do_break();
    abort();
}

[[noreturn]] void __panic_func(const char *file, int line, const char *func, const char *what) {
    os_printf_plus(ROMSTR("PANIC:(%s) at %s:%d\n").value(), what, func, line);
    gdb_do_break();
    abort();
}

} // extern "C"

using __cxxabiv1::__guard;

extern "C" void __cxa_pure_virtual(void) __attribute__ ((__noreturn__));
extern "C" void __cxa_deleted_virtual(void) __attribute__ ((__noreturn__));

void __cxa_pure_virtual(void)
{
    panic();
}

void __cxa_deleted_virtual(void)
{
    panic();
}

typedef struct {
    uint8_t guard;
    uint8_t ps;
} guard_t;

extern "C" int __cxa_guard_acquire(__guard* pg)
{
    uint8_t ps = xt_rsil(15);
    if (reinterpret_cast<guard_t*>(pg)->guard) {
        xt_wsr_ps(ps);
        return 0;
    }
    reinterpret_cast<guard_t*>(pg)->ps = ps;
    return 1;
}

extern "C" void __cxa_guard_release(__guard* pg)
{
    reinterpret_cast<guard_t*>(pg)->guard = 1;
    xt_wsr_ps(reinterpret_cast<guard_t*>(pg)->ps);
}

extern "C" void __cxa_guard_abort(__guard* pg)
{
    xt_wsr_ps(reinterpret_cast<guard_t*>(pg)->ps);
}


namespace std
{
void __throw_bad_function_call()
{
    panic();
}

void __throw_length_error(char const*)
{
    panic();
}

void __throw_bad_alloc()
{
    panic();
}

void __throw_logic_error(const char* str)
{
    panic();
}

void __throw_out_of_range(const char* str)
{
    panic();
}

}
