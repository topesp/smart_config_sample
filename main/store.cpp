#include "store.hpp"
#include "nvs_handle.hpp"
#include "esp_log.h"

#define TAG "store"

#define NVS_NAME "store"

esp_err_t store::got_wifi_info(wifi_info_t *pInfo)
{
    esp_err_t err;
    auto nvsHandler = nvs::open_nvs_handle(NVS_NAME, NVS_READWRITE, &err);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Open nvs handle faile!");
        return err;
    }
    else
    {
        esp_err_t err;
        err = nvsHandler->get_string("pwd", (char *)pInfo->pwd, sizeof(pInfo->pwd));
        if (err != ESP_OK)
            return err;
        err = nvsHandler->get_string("ssid", (char *)pInfo->ssid, sizeof(pInfo->pwd));
        if (err != ESP_OK)
            return err;
        return ESP_OK;
    }
}

esp_err_t store::set_wifi_info(wifi_info_t *pInfo)
{
    esp_err_t err;
    auto nvsHandler = nvs::open_nvs_handle(NVS_NAME, NVS_READWRITE, &err);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Open nvs handle faile!");
        return err;
    }
    else
    {
        esp_err_t err;
        err = nvsHandler->set_string("pwd", (char *)pInfo->pwd);
        if (err != ESP_OK)
            return err;
        err = nvsHandler->set_string("ssid", (char *)pInfo->ssid);
        if (err != ESP_OK)
            return err;
        return ESP_OK;
    }
}