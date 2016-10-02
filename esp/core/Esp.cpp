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
#include "FS.h"
#include "MDNSResponder.h"

extern "C" {
#include <cxxabi.h>
#include "user_interface.h"
#include <smartconfig.h>
#include "umm_malloc.h"

extern const uint32_t __attribute__((section(".ver_number"))) core_version = 0;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABBBCDDD
 *                A : rf cal
 *                B : at parameters
 *                C : rf init data
 *                D : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

} // extern "C"

static const char* WIFIAP_SSID = "ESP8266";
static const char* WIFIAP_PWD = "m8rscript";

static const char* s_panic_file = 0;
static int s_panic_line = 0;
static const char* s_panic_func = 0;

static os_timer_t startupTimer;
static os_timer_t micros_overflow_timer;
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;
static void (*_initializedCB)();
static bool _calledInitializeCB = false;

void initmdns();

IPAddr IPAddr::myIPAddr()
{
    struct ip_info info;
    wifi_get_ip_info(STATION_IF, &info);
    return IPAddr(info.ip.addr);
}

[[noreturn]] void __assert_func(const char *file, int line, const char *func, const char *what) {
    s_panic_file = file;
    s_panic_line = line;
    s_panic_func = func;
    abort();
}

void abort() { while(1) ; }

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

void setUserData(const char* name)
{
    os_printf("Setting device name to '%s'\n", name);
    _gUserData.magic[0] = 'm';
    _gUserData.magic[1] = '8';
    _gUserData.magic[2] = 'r';
    _gUserData.magic[3] = 's';
    // Max name size is 31 characters
    uint16_t size = strlen(name);
    if (size > MaxBonjourNameSize) {
        size = MaxBonjourNameSize;
    }
    memcpy(_gUserData.name, name, size);
    _gUserData.name[size] = '\0';
    m8r::File* file = m8r::FS::sharedFS()->open(".userdata", "w");
    int32_t count = file->write(reinterpret_cast<const char*>(&_gUserData), sizeof(UserSaveData));
    delete file;
    
    initmdns();
}

void getUserData()
{
    m8r::File* file = m8r::FS::sharedFS()->open(".userdata", "r");
    int32_t count = file->read(reinterpret_cast<char*>(&_gUserData), sizeof(UserSaveData));

    if (_gUserData.magic[0] != 'm' || _gUserData.magic[1] != '8' || 
        _gUserData.magic[2] != 'r' || _gUserData.magic[3] != 's') {
        memset(&_gUserData, 0, sizeof(UserSaveData));
    }
}

void ICACHE_FLASH_ATTR hexdump (const char *desc, uint8_t* addr, size_t len)
{
    int i;
    unsigned char buff[17];
    uint8_t* pc = addr;

    // Output description if given.
    if (desc != NULL)
    	os_printf("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
            	os_printf("  %s\n", buff);

            // Output the offset.
            os_printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        os_printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
    	os_printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    os_printf("  %s\n", buff);
}

void ICACHE_FLASH_ATTR smartconfigDone(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD: {
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type* type = (sc_type*) pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        }
        case SC_STATUS_LINK: {
            os_printf("SC_STATUS_LINK\n");
            struct station_config* sta_conf = (struct station_config*) pdata;
            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        }
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                os_printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            smartconfig_stop();
            break;
    }
}

void smartConfig()
{
    wifi_set_opmode(STATION_MODE);
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
    _responder->addService(22, "My Internet Of Things", "m8rscript_shell");
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

void gotStationIP()
{
    if (_initializedCB && !_calledInitializeCB) {
        initmdns();
        _initializedCB();
        _calledInitializeCB = true;
    }
}

static const uint8_t NumWifiTries = 10;
static uint8_t gNumWifiTries = 0;
void wifiEventHandler(System_Event_t *evt)
{
    switch(evt->event) {
        case EVENT_STAMODE_DISCONNECTED: {
            gNumWifiTries++;
            os_printf("Wifi failed to connect %d time%s\n", gNumWifiTries, (gNumWifiTries == 1) ? "" : "s");
            if (gNumWifiTries >= NumWifiTries) {
                os_printf("Wifi connection failed, starting smartconfig\n");
                smartConfig();
            }
            break;
        case EVENT_STAMODE_GOT_IP:
            gotStationIP();
            break;
        }
        default:
            break;
    }
}

static inline char nibbleToHexChar(uint8_t b) { return (b >= 10) ? (b - 10 + 'A') : (b + '0'); }

void initializeSystem(void (*initializedCB)())
{
    _calledInitializeCB = false;
    _initializedCB = initializedCB;
    system_update_cpu_freq(160);
    uart_div_modify(0, UART_CLK_FREQ /115200);
    
    m8r::FS::sharedFS()->mount();
    getUserData();
    do_global_ctors();

    gNumWifiTries = 0;
    wifi_set_opmode(STATION_MODE);
    
    // If we already have our IP we won't get EVENT_STAMODE_GOT_IP since we haven't set
    // up the callback yet. So call it here:
    wifi_set_event_handler_cb(wifiEventHandler);
    if (wifi_station_get_connect_status() == STATION_GOT_IP) {
        gotStationIP();
    }

    os_timer_disarm(&micros_overflow_timer);
    os_timer_setfn(&micros_overflow_timer, (os_timer_func_t*) &micros_overflow_tick, nullptr);
    os_timer_arm(&micros_overflow_timer, 60000, true);
}

uint64_t currentMicroseconds()
{
    uint32_t m = system_get_time();
    uint64_t c = static_cast<uint64_t>(micros_overflow_count) + ((m < micros_at_last_overflow_tick) ? 1 : 0);
    return (c << 32) + m;
}

extern "C" {

void* ICACHE_RAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
	return malloc(size);
}

void ICACHE_RAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
    free(ptr);
}

void* ICACHE_RAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
	void* m = malloc(size);
    memset(m, 0, size);
    return m;
}

size_t xPortGetFreeHeapSize(void)
{
	return umm_free_heap_size();
}

size_t ICACHE_RAM_ATTR xPortWantedSizeAlign(size_t size)
{
    return (size + 3) & ~((size_t) 3);
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
