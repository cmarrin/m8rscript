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

#include "EspGPIOInterface.h"
#include "EspFS.h"
#include "MDNSResponder.h"
#include "SystemInterface.h"
#include "TCP.h"
#include <cstdlib>

#ifndef NDEBUG
#include <gdbstub.h>
#endif

extern "C" {
#include <cxxabi.h>
#include "user_interface.h"
#include <espconn.h>
#include <smartconfig.h>
#include "umm_malloc.h"

//#define NEED_HEXDUMP

extern const uint32_t __attribute__((section(".ver_number"))) core_version = 0;

uint32 user_rf_cal_sector_set(void) {
    extern char flashchip;
    SpiFlashChip *flash = (SpiFlashChip*)(&flashchip + 4);
    // We know that sector size in 4096
    //uint32_t sec_num = flash->chip_size / flash->sector_size;
    uint32_t sec_num = flash->chip_size >> 12;
    return sec_num - 5;
}

} // extern "C"

static const char* WIFIAP_SSID = "ESP8266";
static const char* WIFIAP_PWD = "m8rscript";
static const char* UserDataFilename = ".userdata";

static os_timer_t startupTimer;
static os_timer_t micros_overflow_timer;
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;
static void (*_initializedCB)();
static bool _calledInitializeCB = false;

static m8r::EspFS fileSystem;

m8r::FS* fs() { return &fileSystem; }

m8r::IPAddr m8r::IPAddr::myIPAddr()
{
    struct ip_info info;
    wifi_get_ip_info(STATION_IF, &info);
    return m8r::IPAddr(info.ip.addr);
}

struct LookupHostNameWrapper {
    LookupHostNameWrapper(const char* name, std::function<void (const char* name, m8r::IPAddr)> func)
        : _func(func)
    {
        _espconn.reverse = this;
        espconn_gethostbyname(&_espconn, name, &_ipaddr, callback);
    }
    
    static void callback(const char *name, ip_addr_t *ipaddr, void *arg)
    {
        espconn* ec = reinterpret_cast<espconn*>(arg);
        LookupHostNameWrapper* wrapper = reinterpret_cast<LookupHostNameWrapper*>(ec->reverse);

        m8r::IPAddr ip(ipaddr ? ipaddr->addr : 0);
        wrapper->_func(name, ip);
        delete wrapper;
    }

    std::function<void (const char* name, m8r::IPAddr)> _func;
    ip_addr_t _ipaddr;
    espconn _espconn;
};

void m8r::IPAddr::lookupHostName(const char* name, std::function<void (const char* name, m8r::IPAddr)> func)
{
    // Create a LookupHostNameWrapper to hold all the pieces that need to be passed to espconn_gethostbyname. 
    // LookupHostNameWrapper holds the espconn and ip_addr_t and then the espconn sets its reserved field to
    // point at the LookupHostNameWrapper
    new LookupHostNameWrapper(name, func);
}

void initmdns();

extern "C" {
    int ets_putc(int);
    int ets_vprintf(int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
}

static m8r::TCP* _logTCP = nullptr;

class EspSystemInterface : public m8r::SystemInterface
{
public:
    virtual void vprintf(const char* fmt, va_list) const override;
    virtual m8r::GPIOInterface& gpio() { return _gpio; }
    virtual uint32_t freeMemory() const override { return system_get_free_heap_size(); }
    
private:
    m8r::EspGPIOInterface _gpio;
};

void EspSystemInterface::vprintf(const char* fmt, va_list args) const
{
    size_t fmtlen = ROMstrlen(fmt);
    char* buf = new char[fmtlen + 100];
    ROMvsnprintf(buf, fmtlen + 99, fmt, args);
    
    if (_logTCP) {
        for (uint16_t connection = 0; connection < m8r::TCP::MaxConnections; ++connection) {
            _logTCP->send(connection, buf);
        }
    }
    
    os_printf_plus(buf);
    delete [ ] buf;
}

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    uint32_t m = system_get_time();
    uint64_t c = static_cast<uint64_t>(micros_overflow_count) + ((m < micros_at_last_overflow_tick) ? 1 : 0);
    return (c << 32) + m;
}

static EspSystemInterface _gSystemInterface;

m8r::SystemInterface* system() { return &_gSystemInterface; }

class MyLogTCPDelegate : public m8r::TCPDelegate {
public:
    virtual void TCPevent(m8r::TCP* tcp, m8r::TCPDelegate::Event event, int16_t connectionId, const char* data, int16_t length) override
    {
        if (event == m8r::TCPDelegate::Event::Connected) {
            tcp->send(connectionId, "Start m8rscript Log\n\n");
        }
    }    

private:
};

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

void* ROMmemcpy(void* dst, const void* src, size_t len)
{
    uint8_t* s = (uint8_t*) src;
    uint8_t* d = (uint8_t*) dst;
    while (len--) {
        *d++ = readRomByte(s++);
    }
    return dst;
}

char* ROMCopyString(char* dst, const char* src)
{
    uint8_t* s = (uint8_t*) src;
    char c;
    while ((c = (char) readRomByte(s++))) {
        *dst++ = c;
    }
    *dst = '\0';
    return dst;
}

size_t ROMstrlen(const char* s)
{
    const char* p;
    for (p = s; readRomByte(reinterpret_cast<const uint8_t*>(p)) != '\0'; p++) ;
    return (size_t) (p - s);
}

int ROMstrcmp(const char* s1, const char* s2)
{
    const uint8_t* p1 = reinterpret_cast<const uint8_t*>(s1);
    const uint8_t* p2 = reinterpret_cast<const uint8_t*>(s2);
    
    uint8_t c1;
    uint8_t c2;
    while (true) {
        c1 = readRomByte(p1++);
        c2 = readRomByte(p2++);
        if (c1 != c2) {
            break;
        }
        if (c1 == '\0') {
            return 0;
        }
    }
    return c1 - c2;
}

const char* ROMstrstr(const char* s1, const char* s2)
{
    int i, j;

    if ((s1 == nullptr || s2 == nullptr)) {
        return nullptr;
    }

    for( i = 0; ; i++) {
        char c1 = readRomByte(reinterpret_cast<const uint8_t*>(s1 + i));
        if (c1 == '\0') {
            return nullptr;
        }
        
        char c2 = readRomByte(reinterpret_cast<const uint8_t*>(s2));
        if (c1 == c2) {
            for (j = i; ; j++) {
                c2 = readRomByte(reinterpret_cast<const uint8_t*>(s2 + (j - i)));
                if (c2 == '\0') {
                    return s1 + i;
                }
                c1 = readRomByte(reinterpret_cast<const uint8_t*>(s1 + j));
                if (c1 != c2) {
                    break;
                }
            }
        }
    }
}

int ROMsnprintf (char* s, size_t n, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = ROMvsnprintf(s, n, format, args);
    va_end(args);
    return result;
}

int ROMvsnprintf (char* s, size_t n, const char* format, va_list args)
{
    size_t fmtlen = ROMstrlen(format);
    char* fmtbuf = new char[fmtlen + 1];
    ROMCopyString(fmtbuf, format);
    int result = ets_vsnprintf(s, n, fmtbuf, args);
    delete [ ] fmtbuf;
    return result;
}

void micros_overflow_tick(void* arg) {
    uint32_t m = system_get_time();
    if(m < micros_at_last_overflow_tick) {
        ++micros_overflow_count;
    }
    micros_at_last_overflow_tick = m;
}

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

static void do_global_ctors(void) {
    void (**p)(void) = &__init_array_end;
    while (p != &__init_array_start)
        (*--p)();
}

const uint16_t MaxBonjourNameSize = 31; // Not including trailing '\0'

struct UserSaveData {
    char magic[4];
    char name[MaxBonjourNameSize + 1];
} _gUserData;

void setDeviceName(const char* name)
{
    system()->printf(ROMSTR("Setting device name to '%s'\n"), name);
    uint16_t size = strlen(name);
    if (size > MaxBonjourNameSize) {
        size = MaxBonjourNameSize;
    }
    memcpy(_gUserData.name, name, size);
    _gUserData.name[size] = '\0';
    writeUserData();
    initmdns();
}

void writeUserData()
{
    _gUserData.magic[0] = 'm';
    _gUserData.magic[1] = '8';
    _gUserData.magic[2] = 'r';
    _gUserData.magic[3] = 's';
    m8r::File* file = fileSystem.open(UserDataFilename, "w");
    int32_t count = file->write(reinterpret_cast<const char*>(&_gUserData), sizeof(UserSaveData));
    delete file;
}

void getUserData()
{
    m8r::File* file = fileSystem.open(UserDataFilename, "r");
    int32_t count = file->read(reinterpret_cast<char*>(&_gUserData), sizeof(UserSaveData));

    if (_gUserData.magic[0] != 'm' || _gUserData.magic[1] != '8' || 
        _gUserData.magic[2] != 'r' || _gUserData.magic[3] != 's') {
        memset(&_gUserData, 0, sizeof(UserSaveData));
    }
}

#ifdef NEED_HEXDUMP
void FLASH_ATTR hexdump (const char *desc, uint8_t* addr, size_t len)
{
    int i;
    unsigned char buff[17];
    uint8_t* pc = addr;

    // Output description if given.
    if (desc != NULL)
    	system()->printf(ROMSTR("%s:\n"), desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
            	system()->printf(ROMSTR("  %s\n"), buff);

            // Output the offset.
            system()->printf(ROMSTR("  %04x "), i);
        }

        // Now the hex code for the specific character.
        system()->printf(ROMSTR(" %02x"), pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
    	system()->printf(ROMSTR("   "));
        i++;
    }

    // And print the final ASCII bit.
    system()->printf(ROMSTR("  %s\n"), buff);
}
#endif

void FLASH_ATTR smartconfigDone(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            system()->printf(ROMSTR("SC_STATUS_WAIT\n"));
            break;
        case SC_STATUS_FIND_CHANNEL:
            system()->printf(ROMSTR("SC_STATUS_FIND_CHANNEL\n"));
            break;
        case SC_STATUS_GETTING_SSID_PSWD: {
            system()->printf(ROMSTR("SC_STATUS_GETTING_SSID_PSWD\n"));
            sc_type* type = (sc_type*) pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                system()->printf(ROMSTR("SC_TYPE:SC_TYPE_ESPTOUCH\n"));
            } else {
                system()->printf(ROMSTR("SC_TYPE:SC_TYPE_AIRKISS\n"));
            }
            break;
        }
        case SC_STATUS_LINK: {
            system()->printf(ROMSTR("SC_STATUS_LINK\n"));
            struct station_config* sta_conf = (struct station_config*) pdata;
            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        }
        case SC_STATUS_LINK_OVER:
            system()->printf(ROMSTR("SC_STATUS_LINK_OVER\n"));
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                system()->printf(ROMSTR("Phone ip: %d.%d.%d.%d\n"), phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            smartconfig_stop();
            break;
    }
}

void smartConfig()
{
    smartconfig_stop();
    wifi_set_opmode(STATION_MODE);
    smartconfig_set_type(SC_TYPE_ESPTOUCH);
    smartconfig_start(smartconfigDone);
}

m8r::MDNSResponder* _responder = nullptr;

void initmdns()
{
    if (_responder) {
        delete _responder;
    }

    const char* name = _gUserData.name;
    if (name[0] == '\0') {
        name = "m8rscript";
    }

    _responder = new m8r::MDNSResponder(name);
    _responder->addService(22, "m8r IoT", "m8rscript_shell");
}

void initSoftAP()
{
	wifi_softap_dhcps_stop();
    struct softap_config apConfig;
	wifi_softap_get_config(&apConfig);
    apConfig.channel = 7;
    apConfig.ssid_hidden = 0;
    memset(apConfig.ssid, 0, sizeof(apConfig.ssid));
    strcpy((char*) apConfig.ssid, WIFIAP_SSID);
    apConfig.ssid_len = 10;
    memset(apConfig.password, 0, sizeof(apConfig.password));
    strcpy((char*) apConfig.password, WIFIAP_PWD);
    apConfig.authmode = AUTH_WPA2_PSK;
    apConfig.max_connection = 4;
	apConfig.beacon_interval = 200;
    wifi_softap_set_config(&apConfig);
}

MyLogTCPDelegate _myLogTCPDelegate;

void gotStationIP()
{
    if (_initializedCB && !_calledInitializeCB) {
        initmdns();
        _initializedCB();
        _calledInitializeCB = true;
    }
    _logTCP = m8r::TCP::create(&_myLogTCPDelegate, 23);
}

static const uint8_t NumWifiTries = 10;
static uint8_t gNumWifiTries = 0;
void wifiEventHandler(System_Event_t *evt)
{
    switch(evt->event) {
        case EVENT_STAMODE_CONNECTED:
            system()->printf(ROMSTR("Connected to ssid %s, channel %d\n"), evt->event_info.connected.ssid, evt->event_info.connected.channel);
            break;
        case EVENT_STAMODE_DISCONNECTED: {
            gNumWifiTries++;
            system()->printf(ROMSTR("Wifi failed to connect %d time%s\n"), gNumWifiTries, (gNumWifiTries == 1) ? "" : "s");
            if (gNumWifiTries >= NumWifiTries) {
                gNumWifiTries = 0;
                system()->printf(ROMSTR("Wifi connection failed, starting smartconfig\n"));
                smartConfig();
            }
            break;
        case EVENT_STAMODE_GOT_IP:
            system()->printf(ROMSTR("Got IP, setting up MDS and starting up\n"));
            gotStationIP();
            break;
        }
        default:
            break;
    }
}

static inline char nibbleToHexChar(uint8_t b) { return (b >= 10) ? (b - 10 + 'A') : (b + '0'); }

void startup(void*)
{
    if (!fileSystem.mount()) {
        system()->printf(ROMSTR("SPIFFS filessytem not present, formatting..."));
        if (fileSystem.format()) {
            system()->printf(ROMSTR("succeeded.\n"));
            getUserData();
        } else {
            system()->printf(ROMSTR("FAILED.\n"));
        }
    }

    if (fileSystem.mount()) {
        system()->printf(ROMSTR("Filesystem - total size:%d, used:%d\n"), fileSystem.totalSize(), fileSystem.totalUsed());
    }

    system()->printf(ROMSTR("Starting WiFi:\n"));
    if (wifi_station_get_connect_status() == STATION_GOT_IP) {
        system()->printf(ROMSTR("    already connected, done\n"));
        gotStationIP();
        return;
    }

    gNumWifiTries = 0;
    wifi_set_opmode(STATION_MODE);
    wifi_station_set_auto_connect(1);
    wifi_set_event_handler_cb(wifiEventHandler);
    wifi_station_connect();
    
    struct station_config config;
    wifi_station_get_config(&config);
    if (config.ssid[0] == '\0') {
        system()->printf(ROMSTR("no SSID, running smartconfig\n"));
        smartConfig();
    }
}

void initializeSystem(void (*initializedCB)())
{
    gpio_init();
    
    wifi_station_set_auto_connect(0);
    do_global_ctors();
    _calledInitializeCB = false;
    _initializedCB = initializedCB;
    system_update_cpu_freq(160);
    uart_div_modify(0, UART_CLK_FREQ /115200);

#ifndef NDEBUG
    gdbstub_init();
#endif

    os_timer_disarm(&micros_overflow_timer);
    os_timer_setfn(&micros_overflow_timer, (os_timer_func_t*) &micros_overflow_tick, nullptr);
    os_timer_arm(&micros_overflow_timer, 60000, true);

    os_timer_disarm(&startupTimer);
    os_timer_setfn(&startupTimer, (os_timer_func_t*) &startup, nullptr);
    os_timer_arm(&startupTimer, 2000, false);
}

extern "C" {

void* RAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
	return malloc(size);
}

void RAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
    free(ptr);
}

void* RAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
	void* m = malloc(size);
    memset(m, 0, size);
    return m;
}

size_t xPortGetFreeHeapSize(void)
{
	return umm_free_heap_size();
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

[[noreturn]] void abort()
{
    do {
        *((int*)0) = 0;
    } while(true);
}

[[noreturn]] void __assert_func(const char *file, int line, const char *func, const char *what) {
    os_printf("ASSERT:(%s) at %s:%d\n", what, func, line);
    gdb_do_break();
    abort();
}

[[noreturn]] void __panic_func(const char *file, int line, const char *func, const char *what) {
    os_printf("PANIC:(%s) at %s:%d\n", what, func, line);
    gdb_do_break();
    abort();
}

} // extern "C"

using __cxxabiv1::__guard;

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void * ptr)
{
    free(ptr);
}

void operator delete[](void * ptr)
{
    free(ptr);
}

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
