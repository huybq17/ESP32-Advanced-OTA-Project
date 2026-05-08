#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include "esp_event.h"

// Khai báo một Event Base (Tuyến xe bus) tùy chỉnh cho ứng dụng
ESP_EVENT_DECLARE_BASE(APP_EVENTS);

// Liệt kê các Sự kiện (Trạm dừng) có trên tuyến xe bus này
typedef enum {
    APP_EVENT_WIFI_CONNECTED,    // Được phát (publish) khi mạng đã sẵn sàng
    APP_EVENT_WIFI_DISCONNECTED, // Được phát khi rớt mạng
    APP_EVENT_OTA_STARTED,       // Được phát khi OTA đang chạy
    APP_EVENT_OTA_SUCCESS,       // Được phát khi tải xong OTA
    APP_EVENT_OTA_FAILED         // Được phát khi OTA lỗi
} app_event_id_t;

#endif // APP_EVENTS_H
