#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(APP_EVENTS);

typedef enum {
    APP_EVENT_WIFI_CONNECTED,
    APP_EVENT_WIFI_DISCONNECTED,
    APP_EVENT_OTA_STARTED,
    APP_EVENT_OTA_SUCCESS,
    APP_EVENT_OTA_FAILED
} app_event_id_t;

#endif // APP_EVENTS_H
