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

extern "C" {

#include "Esp.h"
#include "FS.h"
#include <c_types.h>
#include <cxxabi.h>
#include <osapi.h>
#include "user_interface.h"
#include <ets_sys.h>
#include <espconn.h>
#include <smartconfig.h>

extern const uint32_t __attribute__((section(".ver_number"))) core_version = 0;

static const char* WIFIAP_SSID = "ESP8266";
static const char* WIFIAP_PWD = "m8rscript";

static const char* s_panic_file = 0;
static int s_panic_line = 0;
static const char* s_panic_func = 0;

static os_timer_t micros_overflow_timer;
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;

[[noreturn]] void __assert_func(const char *file, int line, const char *func, const char *what) {
    s_panic_file = file;
    s_panic_line = line;
    s_panic_func = func;
    abort();
}

extern void* malloc(size_t size);
extern void free(void* ptr);

void abort() { while(1) ; }

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

// Interface is STATION_IF or SOFTAP_IF
void setIP(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t interface)
{
    struct ip_info info;
    IP4_ADDR(&info.ip, a, b, c, d);
    IP4_ADDR(&info.gw, a, b, c, d);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    wifi_set_ip_info(interface, &info);
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

void initmdns(const char* hostname, uint8_t interface)
{
    struct mdns_info mdnsInfo;
    struct ip_info ipconfig;
    wifi_get_ip_info(interface, &ipconfig);
    os_memset(&mdnsInfo, 0, sizeof(mdnsInfo));
    mdnsInfo.host_name = (char*) hostname; 
    mdnsInfo.server_name = (char*) "m8rscript_server";
    mdnsInfo.ipAddr = ipconfig.ip.addr;
    mdnsInfo.server_port = 80; 
    mdnsInfo.txt_data[0] = (char*) "version = now"; 
    mdnsInfo.txt_data[1] = (char*) "user1 = data1"; 
    mdnsInfo.txt_data[2] = (char*) "user2 = data2";
    espconn_mdns_init(&mdnsInfo);
    espconn_mdns_enable();
    os_printf("The mDNS responder is running at %s.local.\n", hostname);
}

void initSoftAP()
{
	wifi_softap_dhcps_stop();
    struct softap_config apConfig;
	wifi_softap_get_config(&apConfig);
    apConfig.channel = 7;
    apConfig.ssid_hidden = 0;
    os_memset(apConfig.ssid, 0, sizeof(apConfig.ssid));
    strcpy((char*) apConfig.ssid, WIFIAP_SSID);
    apConfig.ssid_len = 10;
    os_memset(apConfig.password, 0, sizeof(apConfig.password));
    strcpy((char*) apConfig.password, WIFIAP_PWD);
    apConfig.authmode = AUTH_WPA2_PSK;
    apConfig.max_connection = 4;
	apConfig.beacon_interval = 200;
    wifi_softap_set_config(&apConfig);
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
            initmdns("m8rscript", STATION_IF);
            break;
        }
        default:
            break;
    }
}

void initializeSystem()
{
    uart_div_modify(0, UART_CLK_FREQ /115200);
    
    do_global_ctors();

    // Set DHCP Name
    uint8_t hwaddr[6] = { 0 };
	wifi_get_macaddr(STATION_IF, hwaddr);
    char hostname[8];
    static const char* hex = "0123456789ABCDEF";
    hostname[0] = 'E';
    hostname[1] = 'S';
    hostname[2] = 'P';
    hostname[3] = hex[hwaddr[4] >> 4];
    hostname[4] = hex[hwaddr[4] && 0x0f];
    hostname[5] = hex[hwaddr[5] >> 4];
    hostname[6] = hex[hwaddr[5] && 0x0f];
    hostname[7] = '\0';
    wifi_station_set_hostname(hostname);

    //initSoftAP();
    
    //setIP(192, 168, 22, 1, SOFTAP_IF);

//    struct dhcps_lease dhcp_lease;
//    IP4_ADDR(&dhcp_lease.start_ip, 192, 168, 22, 2);
//    IP4_ADDR(&dhcp_lease.end_ip, 192, 168, 22, 5);
//    wifi_softap_set_dhcps_lease(&dhcp_lease);
//
//    wifi_softap_dhcps_start();
    
    //initmdns("m8rscript", SOFTAP_IF);
    
    gNumWifiTries = 0;

    wifi_softap_dhcps_stop();
    wifi_set_opmode(STATION_MODE);
    wifi_set_event_handler_cb(wifiEventHandler);

    os_timer_disarm(&micros_overflow_timer);
    os_timer_setfn(&micros_overflow_timer, (os_timer_func_t*) &micros_overflow_tick, 0);
    os_timer_arm(&micros_overflow_timer, 60000, 1 /* REPEAT */);

    //spiffs_mount();
}

uint64_t currentMicroseconds()
{
    uint32_t m = system_get_time();
    uint64_t c = static_cast<uint64_t>(micros_overflow_count) + ((m < micros_at_last_overflow_tick) ? 1 : 0);
    return (c << 32) + m;
}


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
    os_memset(m, 0, size);
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
}
