#ifndef STORE_HPP_
#define STORE_HPP_

#include <stdint.h>

namespace store
{
    typedef int esp_err_t;
    typedef struct wifi_info
    {
        uint8_t ssid[32];
        uint8_t pwd[64];
    } wifi_info_t;

    esp_err_t got_wifi_info(wifi_info_t *p);

    esp_err_t set_wifi_info(wifi_info_t *p);

};

#endif