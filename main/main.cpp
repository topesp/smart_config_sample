

#include "nvs_flash.h"
#include <esp_netif.h>
#include "esp_event.h"

#include "wifi.hpp"

#define TAG "smart_config:mian"


extern "C" void app_main(void)
{
    TaskHandle_t wifiTaskHandle;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(wifi_task, "wifi_task", 4096 * 2, NULL, 5, &wifiTaskHandle);

}

