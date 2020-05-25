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
#include <cstring>

using namespace m8r;

static constexpr int IPV4_GOTIP_BIT = BIT0;

esp_err_t RtosWifi::eventHandler(void* ctx, system_event_t* event)
{
    RtosWifi* self = reinterpret_cast<RtosWifi*>(ctx);
    
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(self->_eventGroup, IPV4_GOTIP_BIT);
        printf("***** got ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        break;
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
    case SYSTEM_EVENT_STA_DISCONNECTED: {
        system_event_sta_disconnected_t *disconnected = &event->event_info.disconnected;
        printf("***** WiFi disconnected, ssid:%s, ssid_len:%d, bssid:" MACSTR ", reason:%d\n",
            disconnected->ssid, disconnected->ssid_len, MAC2STR(disconnected->bssid), disconnected->reason);
                   
        if (disconnected->reason == WIFI_REASON_MIC_FAILURE) {
            // Try to restart on message integrity check error
            printf("      Disconnect because of message integrity check failure.\n");
        }
        else if (disconnected->reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            printf("      Disconnect because of unsupported rate, switching to lower rate.\n");
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        printf("      Attempting to reconnect...\n");
        esp_wifi_connect();
        xEventGroupClearBits(self->_eventGroup, IPV4_GOTIP_BIT);
        break;
    }
    default:
        break;
    }
    return ESP_OK;
}

void RtosWifi::start()
{
    tcpip_adapter_init();
    _eventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(eventHandler, this));

    wifi_init_config_t initConfig;
    
    initConfig.event_handler = &esp_event_send;
    initConfig.osi_funcs = NULL;
    initConfig.qos_enable = WIFI_QOS_ENABLED;
    initConfig.ampdu_rx_enable = WIFI_AMPDU_RX_ENABLED;
    initConfig.rx_ampdu_buf_len = WIFI_AMPDU_RX_AMPDU_BUF_LEN;
    initConfig.rx_ampdu_buf_num = WIFI_AMPDU_RX_AMPDU_BUF_NUM;
    initConfig.amsdu_rx_enable = WIFI_AMSDU_RX_ENABLED;
    initConfig.rx_ba_win = WIFI_AMPDU_RX_BA_WIN;
    initConfig.rx_max_single_pkt_len = WIFI_RX_MAX_SINGLE_PKT_LEN;
    initConfig.rx_buf_len = WIFI_HW_RX_BUFFER_LEN;
    initConfig.rx_buf_num = CONFIG_ESP8266_WIFI_RX_BUFFER_NUM;
    initConfig.left_continuous_rx_buf_num = CONFIG_ESP8266_WIFI_LEFT_CONTINUOUS_RX_BUFFER_NUM;
    initConfig.rx_pkt_num = CONFIG_ESP8266_WIFI_RX_PKT_NUM;
    initConfig.tx_buf_num = CONFIG_ESP8266_WIFI_TX_PKT_NUM;
    initConfig.nvs_enable = WIFI_NVS_ENABLED;
    initConfig.nano_enable = 0;
    initConfig.magic = WIFI_INIT_CONFIG_MAGIC;
    
    ESP_ERROR_CHECK(esp_wifi_init(&initConfig));
    
    _inited = true;
    
    wifi_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(reinterpret_cast<char*>(config.sta.ssid), "marrin");
    strcpy(reinterpret_cast<char*>(config.sta.password), "orion741");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for IP address
    printf("***** Waiting for WiFi to connect...\n");
    xEventGroupWaitBits(_eventGroup, IPV4_GOTIP_BIT, false, true, portMAX_DELAY);
    printf("***** ... WiFi connected.\n");
}
