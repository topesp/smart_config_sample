
#include "wifi.hpp"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_smartconfig.h"

#define TAG "smart_config:wifi"

EventGroupHandle_t wifiEventGroupHandle;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static void wifi_handler(void *event_handler_arg,
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
    wifiEventGroupHandle = xEventGroupCreate();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, smart_config_handler, NULL);
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
        xTaskCreate(smart_config_task, "smartconfig_task", 4096, NULL, 3, NULL);
        break;
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "wifi station connected");
        break;
    case WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, "wifi station Stop");
        vEventGroupDelete(wifiEventGroupHandle);
        wifiEventGroupHandle = NULL;
        break;
    default:
        break;
    }
}

static void smart_config_handler(void *event_handler_arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data)
{

    switch (event_id)
    {
    case SC_EVENT_GOT_SSID_PSWD:
        ESP_LOGI(TAG, "Smartconfig got ssid and pswd");
        break;
    case SC_EVENT_SEND_ACK_DONE:
        ESP_LOGI(TAG, "Smartconfig Ack Done");
        xEventGroupSetBits(wifiEventGroupHandle, ESPTOUCH_DONE_BIT);
        break;
    default:
        ESP_LOGI(TAG, "unhandler envent : %ld", event_id);
        break;
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
            ESP_LOGI(TAG, "smart_config_task : station connnected");
        }
        else if (eventWaitBits & ESPTOUCH_DONE_BIT)
        {
            esp_smartconfig_stop();
            vTaskDelete(NULL);
            ESP_LOGI(TAG, "smart_config_task : esp touch done");
        }
        else
        {
            ESP_LOGI(TAG, "smart_config_task : wait timeout");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}