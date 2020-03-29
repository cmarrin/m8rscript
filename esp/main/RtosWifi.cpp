/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "RtosWifi.h"

using namespace m8r;

RtosWifi::RtosWifi()
{
    _initConfig.event_handler = &esp_event_send;
    _initConfig.osi_funcs = NULL;
    _initConfig.qos_enable = WIFI_QOS_ENABLED;
    _initConfig.ampdu_rx_enable = WIFI_AMPDU_RX_ENABLED;
    _initConfig.rx_ampdu_buf_len = WIFI_AMPDU_RX_AMPDU_BUF_LEN;
    _initConfig.rx_ampdu_buf_num = WIFI_AMPDU_RX_AMPDU_BUF_NUM;
    _initConfig.amsdu_rx_enable = WIFI_AMSDU_RX_ENABLED;
    _initConfig.rx_ba_win = WIFI_AMPDU_RX_BA_WIN;
    _initConfig.rx_max_single_pkt_len = WIFI_RX_MAX_SINGLE_PKT_LEN;
    _initConfig.rx_buf_len = WIFI_HW_RX_BUFFER_LEN;
    _initConfig.rx_buf_num = CONFIG_ESP8266_WIFI_RX_BUFFER_NUM;
    _initConfig.left_continuous_rx_buf_num = CONFIG_ESP8266_WIFI_LEFT_CONTINUOUS_RX_BUFFER_NUM;
    _initConfig.rx_pkt_num = CONFIG_ESP8266_WIFI_RX_PKT_NUM;
    _initConfig.tx_buf_num = CONFIG_ESP8266_WIFI_TX_PKT_NUM;
    _initConfig.nvs_enable = WIFI_NVS_ENABLED;
    _initConfig.nano_enable = 0;
    _initConfig.magic = WIFI_INIT_CONFIG_MAGIC;
    
    esp_err_t err = esp_wifi_init(&_initConfig);
    if (err != ESP_OK) {
        printf("***** esp_wifi_init failed (%d)\n", err);
        return;
    }
    
    _inited = true;
}



#if 0
// vvvvvvvvvv From ESP8266_RTOS_SDK simple_wifi example
extern "C" {
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
}

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_MODE_AP   TRUE //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_WIFI_SSID      "ESP_M8R_01"
#define EXAMPLE_ESP_WIFI_PASS      "marrin99"
#define EXAMPLE_MAX_STA_CONN       4

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "simple wifi";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_softap()
{





    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);





//    wifi_event_group = xEventGroupCreate();
//
//    tcpip_adapter_init();
//    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
//
//    wifi_init_config_t cfg = {
//        event_handler : &esp_event_send,
//        osi_funcs : NULL,
//        qos_enable : WIFI_QOS_ENABLED,
//        ampdu_rx_enable : WIFI_AMPDU_RX_ENABLED,
//        rx_ba_win : WIFI_AMPDU_RX_BA_WIN,
//        rx_ampdu_buf_num : WIFI_AMPDU_RX_AMPDU_BUF_NUM,
//        rx_ampdu_buf_len : WIFI_AMPDU_RX_AMPDU_BUF_LEN,
//        rx_max_single_pkt_len : WIFI_RX_MAX_SINGLE_PKT_LEN,
//        rx_buf_len : WIFI_HW_RX_BUFFER_LEN,
//        amsdu_rx_enable : WIFI_AMSDU_RX_ENABLED,
//        rx_buf_num : CONFIG_ESP8266_WIFI_RX_BUFFER_NUM,
//        rx_pkt_num : CONFIG_ESP8266_WIFI_RX_PKT_NUM,
//        left_continuous_rx_buf_num : CONFIG_ESP8266_WIFI_LEFT_CONTINUOUS_RX_BUFFER_NUM,
//        tx_buf_num : CONFIG_ESP8266_WIFI_TX_PKT_NUM,
//        nvs_enable : WIFI_NVS_ENABLED,
//        nano_enable : 0,
//        magic : WIFI_INIT_CONFIG_MAGIC
//    };
//    
//    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//    wifi_config_t wifi_config = {
//        ap : {
//            ssid : EXAMPLE_ESP_WIFI_SSID,
//            password : EXAMPLE_ESP_WIFI_PASS,
//            ssid_len : strlen(EXAMPLE_ESP_WIFI_SSID),
//            max_connection : EXAMPLE_MAX_STA_CONN,
//            authmode : WIFI_AUTH_WPA_WPA2_PSK
//        },
//    };
//    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
//        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//    }
//
//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
//    ESP_ERROR_CHECK(esp_wifi_start());
//
//    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
//             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void wifi_example_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
#if EXAMPLE_ESP_WIFI_MODE_AP
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
#else
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
#endif /*EXAMPLE_ESP_WIFI_MODE_AP*/

}

// ^^^^^^^^^^ From ESP8266_RTOS_SDK simple_wifi example

#endif
