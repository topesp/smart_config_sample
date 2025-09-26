#ifndef __wifi_hpp__
#define __wifi_hpp__

#include "esp_event.h"

extern EventGroupHandle_t wifiEventGroupHandle;

void wifi_task(void *);


#endif