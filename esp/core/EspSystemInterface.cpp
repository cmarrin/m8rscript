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

#include "EspGPIOInterface.h"
#include "EspTaskManager.h"
#include "EspTCP.h"
#include "EspUDP.h"
#include "MDNSResponder.h"
#include "MString.h"
#include "SystemInterface.h"
#include "TCP.h"
#include <cstdlib>

#ifndef USE_LITTLEFS
#include "SpiffsFS.h"
#else
#include "LittleFS.h"
#endif

#include <espconn.h>
#include <smartconfig.h>

static const char* WIFIAP_SSID = "ESP8266";
static const char* WIFIAP_PWD = "m8rscript";
static const char* UserDataFilename = ".userdata";

static os_timer_t startupTimer;
static os_timer_t micros_overflow_timer;
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;
static void (*_initializedCB)();
static bool _calledInitializeCB = false;

void micros_overflow_tick(void* arg) {
    uint32_t m = system_get_time();
    if(m < micros_at_last_overflow_tick) {
        ++micros_overflow_count;
    }
    micros_at_last_overflow_tick = m;
}

m8r::IPAddr m8r::IPAddr::myIPAddr()
{
    struct ip_info info;
    wifi_get_ip_info(STATION_IF, &info);
    return m8r::IPAddr(info.ip.addr);
}

struct LookupHostNameWrapper {
    LookupHostNameWrapper() { }

    void init(const char* name, std::function<void (const char* name, m8r::IPAddr)> func)
    {
        _func = func;
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

LookupHostNameWrapper _lookupHostNameWrapper;

void m8r::IPAddr::lookupHostName(const char* name, std::function<void (const char* name, m8r::IPAddr)> func)
{
    // Init the LookupHostNameWrapper to hold all the pieces that need to be passed to espconn_gethostbyname. 
    // LookupHostNameWrapper holds the espconn and ip_addr_t and then the espconn sets its reserved field to
    // point at the LookupHostNameWrapper
    _lookupHostNameWrapper.init(name, func);
}

void initmdns();

static m8r::Mad<m8r::TCP> _logTCP;

void setDeviceName(const char*);

class EspSystemInterface : public m8r::SystemInterface
{
public:
    virtual void vprintf(m8r::ROMString fmt, va_list) const override;
    virtual void setDeviceName(const char* name) { ::setDeviceName(name); }
    
    virtual m8r::FS* fileSystem() override { return &_fileSystem; }
    virtual m8r::GPIOInterface* gpio() override { return &_gpio; }
    virtual m8r::TaskManager* taskManager() override { return &_taskManager; };
    
    virtual m8r::Mad<m8r::TCP> createTCP(m8r::TCPDelegate* delegate, uint16_t port, m8r::IPAddr ip = m8r::IPAddr()) override
    {
        m8r::Mad<m8r::EspTCP> tcp = m8r::Mad<m8r::EspTCP>::create(m8r::MemoryType::Network);
        tcp->init(delegate, port, ip);
        return tcp;
    }
    
    virtual m8r::Mad<m8r::UDP> createUDP(m8r::UDPDelegate* delegate, uint16_t port) override
    {
        m8r::Mad<m8r::EspUDP> udp = m8r::Mad<m8r::EspUDP>::create(m8r::MemoryType::Network);
        udp->init(delegate, port);
        return udp;
    }

private:
    m8r::EspGPIOInterface _gpio;
#ifndef USE_LITTLEFS
    m8r::SpiffsFS _fileSystem;
#else
    m8r::LittleFS _fileSystem;
#endif
    m8r::EspTaskManager _taskManager;
};

void EspSystemInterface::vprintf(m8r::ROMString fmt, va_list args) const
{
    m8r::String s = m8r::String::vformat(fmt, args);
    
    if (_logTCP.valid()) {
        for (uint16_t connection = 0; connection < m8r::TCP::MaxConnections; ++connection) {
            _logTCP->send(connection, s.c_str());
        }
    }
    
    os_printf_plus(s.c_str());
}

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    uint32_t m = system_get_time();
    uint64_t c = static_cast<uint64_t>(micros_overflow_count) + ((m < micros_at_last_overflow_tick) ? 1 : 0);
    return (c << 32) + m;
}

static EspSystemInterface _gSystemInterface;

m8r::SystemInterface* m8r::SystemInterface::get() { return &_gSystemInterface; }

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

const uint16_t MaxBonjourNameSize = 31; // Not including trailing '\0'

struct UserSaveData {
    char magic[4];
    char name[MaxBonjourNameSize + 1];
} _gUserData;

void setDeviceName(const char* name)
{
    m8r::system()->printf(ROMSTR("Setting device name to '%s'\n"), name);
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
    m8r::Mad<m8r::File> file = m8r::system()->fileSystem()->open(UserDataFilename, m8r::FS::FileOpenMode::Write);
    int32_t count = file->write(reinterpret_cast<const char*>(&_gUserData), sizeof(UserSaveData));
    file.destroy(m8r::MemoryType::Native);
}

void getUserData()
{
    m8r::Mad<m8r::File> file = m8r::system()->fileSystem()->open(UserDataFilename, m8r::FS::FileOpenMode::Read);
    int32_t count = file->read(reinterpret_cast<char*>(&_gUserData), sizeof(UserSaveData));
    file.destroy(m8r::MemoryType::Native);

    if (_gUserData.magic[0] != 'm' || _gUserData.magic[1] != '8' || 
        _gUserData.magic[2] != 'r' || _gUserData.magic[3] != 's') {
        memset(&_gUserData, 0, sizeof(UserSaveData));
    }
}

void FLASH_ATTR smartconfigDone(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            m8r::system()->printf(ROMSTR("SC_STATUS_WAIT\n"));
            break;
        case SC_STATUS_FIND_CHANNEL:
            m8r::system()->printf(ROMSTR("SC_STATUS_FIND_CHANNEL\n"));
            break;
        case SC_STATUS_GETTING_SSID_PSWD: {
            m8r::system()->printf(ROMSTR("SC_STATUS_GETTING_SSID_PSWD\n"));
            sc_type* type = (sc_type*) pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                m8r::system()->printf(ROMSTR("SC_TYPE:SC_TYPE_ESPTOUCH\n"));
            } else {
                m8r::system()->printf(ROMSTR("SC_TYPE:SC_TYPE_AIRKISS\n"));
            }
            break;
        }
        case SC_STATUS_LINK: {
            m8r::system()->printf(ROMSTR("SC_STATUS_LINK\n"));
            struct station_config* sta_conf = (struct station_config*) pdata;
            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        }
        case SC_STATUS_LINK_OVER:
            m8r::system()->printf(ROMSTR("SC_STATUS_LINK_OVER\n"));
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                m8r::system()->printf(ROMSTR("Phone ip: %d.%d.%d.%d\n"), phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
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

m8r::Mad<m8r::MDNSResponder> _responder;

void initmdns()
{
    if (_responder.valid()) {
        _responder.destroy(m8r::MemoryType::Network);
        _responder.reset();
    }

    const char* name = _gUserData.name;
    if (name[0] == '\0') {
        name = "m8rscript";
    }

    _responder = m8r::Mad<m8r::MDNSResponder>::create(m8r::MemoryType::Network);
    _responder->init(name);
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
    _logTCP = m8r::system()->createTCP(&_myLogTCPDelegate, 23);
}

static const uint8_t NumWifiTries = 10;
static uint8_t gNumWifiTries = 0;
void wifiEventHandler(System_Event_t *evt)
{
    switch(evt->event) {
        case EVENT_STAMODE_CONNECTED:
            m8r::system()->printf(ROMSTR("Connected to ssid %s, channel %d\n"), evt->event_info.connected.ssid, evt->event_info.connected.channel);
            break;
        case EVENT_STAMODE_DISCONNECTED: {
            gNumWifiTries++;
            m8r::system()->printf(ROMSTR("Wifi failed to connect %d time%s\n"), gNumWifiTries, (gNumWifiTries == 1) ? "" : "s");
            if (gNumWifiTries >= NumWifiTries) {
                gNumWifiTries = 0;
                m8r::system()->printf(ROMSTR("Wifi connection failed, starting smartconfig\n"));
                smartConfig();
            }
            break;
        case EVENT_STAMODE_GOT_IP:
            m8r::system()->printf(ROMSTR("Got IP, setting up MDS and starting up\n"));
            gotStationIP();
            break;
        }
        default:
            break;
    }
}

void startup(void*)
{
    m8r::system()->printf(ROMSTR("Starting WiFi:\n"));
    if (wifi_station_get_connect_status() == STATION_GOT_IP) {
        m8r::system()->printf(ROMSTR("    already connected, done\n"));
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
        m8r::system()->printf(ROMSTR("no SSID, running smartconfig\n"));
        smartConfig();
    }
}

void initializeSystem(void (*initializedCB)())
{
    gpio_init();

    // Seed the random number generator
    srand(RANDOM_REG32);
    
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

static s32_t spiffsRead(u32_t addr, u32_t size, u8_t *dst)
{
    return (flashmem_read(dst, addr, size) == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_READABLE;
}

static s32_t spiffsWrite(u32_t addr, u32_t size, u8_t *src)
{
    return (flashmem_write(src, addr, size) == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_WRITABLE;
}

static s32_t spiffsErase(u32_t addr, u32_t size)
{
    u32_t firstSector = flashmem_get_sector_of_address(addr);
    u32_t lastSector = firstSector;
    while(firstSector <= lastSector) {
        if(!flashmem_erase_sector(firstSector++)) {
            return SPIFFS_ERR_INTERNAL;
        }
    }
    return SPIFFS_OK;
}

#ifndef USE_LITTLEFS

void m8r::SpiffsFS::setConfig(spiffs_config& config, const char*)
{
    memset(&config, 0, sizeof(config));
    config.hal_read_f = spiffsRead;
    config.hal_write_f = spiffsWrite;
    config.hal_erase_f = spiffsErase;
}

#else

static int lfs_flash_read(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size) {
    uint32_t addr = (block * 256) + off;
    return spiffsRead(addr, size, static_cast<uint8_t*>(dst)) == SPIFFS_OK ? 0 : -1;
}

static int lfs_flash_prog(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = (block * 256) + off;
    const uint8_t *src = reinterpret_cast<const uint8_t *>(buffer);
    return spiffsWrite(addr, size, const_cast<uint8_t*>(src)) == SPIFFS_OK ? 0 : -1;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = (block * 256);
    uint32_t size = 256;
    return spiffsErase(addr, size) == SPIFFS_OK ? 0 : -1;
}

static int lfs_flash_sync(const struct lfs_config *c) {
    /* NOOP */
    (void) c;
    return 0;
}

void m8r::LittleFS::setConfig(lfs_config& config, const char* name)
{
    lfs_size_t blockSize = 256;
    lfs_size_t size = 3 * 1024 * 1024;
    
    memset(&config, 0, sizeof(config));
    //config.context = (void*) this;
    config.read = lfs_flash_read;
    config.prog = lfs_flash_prog;
    config.erase = lfs_flash_erase;
    config.sync = lfs_flash_sync;
    config.read_size = 64;
    config.prog_size = 64;
    config.block_size =  blockSize;
    config.block_count = blockSize ? size / blockSize: 0;
    config.block_cycles = 16; // TODO - need better explanation
    config.cache_size = 64;
    config.lookahead_size = 64;
    config.read_buffer = nullptr;
    config.prog_buffer = nullptr;
    config.lookahead_buffer = nullptr;
    config.name_max = 0;
    config.file_max = 0;
    config.attr_max = 0;
}

#endif
