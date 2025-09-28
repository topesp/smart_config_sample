#include <cstring>

#include "wifi.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_smartconfig.h"

#include "store.hpp"

#define TAG "smart_config:wifi"

typedef ip_event_got_ip_t *ip_event_got_ip_handle_t;

EventGroupHandle_t wifiEventGroupHandle;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static void wifi_handler(void *event_handler_arg,
                         esp_event_base_t event_base,
                         int32_t event_id,
                         void *event_data);

static void ip_event_handler(void *event_handler_arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data);

static void smart_config_handler(void *event_handler_arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data);

static void smart_config_task(void *);

void wifi_task(void *)
{
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    
    wifiEventGroupHandle = xEventGroupCreate();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, smart_config_handler, NULL);
    esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, NULL);
    ESP_ERROR_CHECK(esp_wifi_start());
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(2 * 1000));
    }
}

static void wifi_handler(void *event_handler_arg,
                         esp_event_base_t event_base,
                         int32_t event_id,
                         void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "wifi station start");
        // 判断是否需要配网
        // Start smart config
        // nvs got ssid and password
        xTaskCreate(smart_config_task, "smartconfig_task", 4096, NULL, 3, NULL);
        break;
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "wifi station connected");
        break;
    case WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, "wifi station Stop");
        // vEventGroupDelete(wifiEventGroupHandle);
        // wifiEventGroupHandle = NULL;
        break;
    default:
        break;
    }
}

static void ip_event_handler(void *event_handler_arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_handle_t pInfo = (ip_event_got_ip_handle_t)event_data;
        ESP_LOGI(TAG, IPSTR, IP2STR(&pInfo->ip_info.ip));
        xEventGroupSetBits(wifiEventGroupHandle, CONNECTED_BIT);
    }
}

static void smart_config_handler(void *event_handler_arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data)
{
    if (event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(TAG, "Smartconfig got ssid and pswd");
        smartconfig_event_got_ssid_pswd_t *pSmartConfig = (smartconfig_event_got_ssid_pswd_t *)event_data;
        store::wifi_info_t info;

        uint8_t cellphone_ip[4];
        wifi_config_t conf;
        bzero(&info, sizeof(store::wifi_info_t));
        bzero(&conf, sizeof(wifi_config_t));
        memcpy(info.ssid, pSmartConfig->ssid, sizeof(pSmartConfig->ssid));
        memcpy(info.pwd, pSmartConfig->password, sizeof(pSmartConfig->password));
        memcpy(cellphone_ip, pSmartConfig->cellphone_ip, sizeof(pSmartConfig->cellphone_ip));

        ESP_LOGI(TAG, "ssid : %s ; pwd : %s", info.ssid, info.pwd);
        ESP_LOGI(TAG, "Receive from : %d.%d.%d.%d", cellphone_ip[0], cellphone_ip[1], cellphone_ip[2], cellphone_ip[3]);
        auto res = store::set_wifi_info(&info);
        if(res != ESP_OK)
        {
            ESP_LOGE(TAG, "Strore wifi info error");
        }
        memcpy(conf.sta.ssid, pSmartConfig->ssid, sizeof(conf.sta.ssid));
        memcpy(conf.sta.password, pSmartConfig->password, sizeof(conf.sta.password));

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &conf));
        esp_wifi_connect();
    }
    else if (event_id == SC_EVENT_SEND_ACK_DONE)
    {
        ESP_LOGI(TAG, "Smartconfig Ack Done");
        xEventGroupSetBits(wifiEventGroupHandle, ESPTOUCH_DONE_BIT);
    }
}

static void smart_config_task(void *pvParameters)
{
    EventBits_t eventWaitBits;
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    while (1)
    {
        eventWaitBits = xEventGroupWaitBits(wifiEventGroupHandle, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, pdMS_TO_TICKS(60 * 1000));
        ESP_LOGI(TAG, "smart_config_task %ld", eventWaitBits);
        if (eventWaitBits & CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "Wifi Connected!");
        }
        else if (eventWaitBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smartconfig will stop!");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
        else
        {
            ESP_LOGI(TAG, "smart_config_task : wait timeout");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}