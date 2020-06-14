/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "RtosWifi.h"

#include "esp_event_loop.h"
#include "MString.h"
#include "SystemTime.h"
#include <nvs_flash.h>
#include <cstring>

using namespace m8r;

static bool SetupTestSTAConnection = false;

// Wait a total of 10 seconds to connect
static m8r::Duration WiFiConnectWaitDuration = 500ms;
static int WiFiConnectWaitCycles = 20;

static constexpr int IPV4_GOTIP_BIT = BIT0;

esp_err_t RtosWifi::eventHandler(void* ctx, system_event_t* event)
{
    RtosWifi* self = reinterpret_cast<RtosWifi*>(ctx);

    switch(event->event_id) {
    case SYSTEM_EVENT_SCAN_DONE: {
        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        wifi_ap_record_t* list = new wifi_ap_record_t [apCount];
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));
       
        for (uint16_t i = 0; i < apCount; ++i) {
            self->_ssidList.push_back(list[i].ssid);
        }
        break;
    }
    
    case SYSTEM_EVENT_STA_START:
        if (self->_state == State::InitialTry || self->_state == State::Retry) {
            esp_wifi_connect();
        }
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(self->_eventGroup, IPV4_GOTIP_BIT);
        printf("***** got ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED: {
        system_event_sta_disconnected_t *disconnected = &event->event_info.disconnected;
        printf("***** WiFi disconnected, ssid:%s, ssid_len:%d, bssid:" MACSTR ", reason:%d\n",
            disconnected->ssid, disconnected->ssid_len, MAC2STR(disconnected->bssid), disconnected->reason);
        
        if (self->_state != State::InitialTry && self->_state != State::Retry) {
            break;
        }
        
        if (disconnected->reason == WIFI_REASON_MIC_FAILURE) {
            // Try to restart on message integrity check error
            printf("      Disconnect because of message integrity check failure.\n");
        }
        else if (disconnected->reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            printf("      Disconnect because of unsupported rate, switching to lower rate.\n");
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
        }
        printf("      Attempting to reconnect...\n");
        esp_wifi_connect();
        xEventGroupClearBits(self->_eventGroup, IPV4_GOTIP_BIT);
        break;
    }

    case SYSTEM_EVENT_AP_STACONNECTED:
        printf("***** station:" MACSTR " join, AID=%d\n",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        printf("***** station:" MACSTR "leave, AID=%d\n",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
    return ESP_OK;
}

bool RtosWifi::waitForConnect()
{
    for (int i = 0; i < WiFiConnectWaitCycles; ++i) {
        if (xEventGroupWaitBits(_eventGroup, IPV4_GOTIP_BIT, false, true, pdMS_TO_TICKS(WiFiConnectWaitDuration.ms()))) {
            return true;
        }
        printf("   still waiting...\n");
    }
    return false;
}

void RtosWifi::connectToSTA(const char* ssid, const char* pwd)
{
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(reinterpret_cast<char*>(config.sta.ssid), ssid);
    strcpy(reinterpret_cast<char*>(config.sta.password), pwd);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
}

void RtosWifi::scanForNetworks()
{
    wifi_scan_config_t config;
    memset(&config, 0, sizeof(config));
    config.show_hidden = true;
    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, true));
    
    // For now just print them out
    printf("SSID List:\n");
    for (const auto& it : _ssidList) {
        printf("    '%s'\n", it.c_str());
    }
    printf("====\n");
}

bool RtosWifi::setupConnection(const char* name)
{
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(reinterpret_cast<char*>(config.ap.ssid), name);
    config.ap.max_connection = 1;
    config.ap.authmode = WIFI_AUTH_OPEN;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("***** Started AP named '%s'\n", name);
    
    // FIXME: Now start a webserver and wait for a network to be setup
    return false;
}

void RtosWifi::start()
{
    _eventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(eventHandler, this));

    nvs_flash_init();
    tcpip_adapter_init();

    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initConfig));
    
    // First try to connect to the existing STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    if (SetupTestSTAConnection) {
        // Make a connection for testing
        wifi_config_t config;
        memset(&config, 0, sizeof(config));
        strcpy(reinterpret_cast<char*>(config.sta.ssid), "marrin");
        strcpy(reinterpret_cast<char*>(config.sta.password), "orion741");
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
    }
    
    // Wait for IP address
    printf("** Waiting for WiFi to connect...\n");
    if (!waitForConnect()) {
        // Failed to connect, scan
        _state = State::Scan;
        esp_wifi_disconnect();
        scanForNetworks();
        
        // Setup Network
        if (!setupConnection("MyCoolESPAP")) {
            esp_wifi_disconnect();
            esp_wifi_stop();
            esp_wifi_deinit();
            printf("***** ERROR: WiFi failed to connect, disabling.\n");
            return;
        }
    }
    
    printf("***** WiFi connected.\n");
}
