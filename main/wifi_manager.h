#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H


#define WIFI_SSID      "Phong 102"
#define WIFI_PASS      "leanh102"

// Initialize and connect to Wi-Fi. This module operates independently and
// will publish events via esp_event_loop.
void wifi_manager_init(void);

#endif // WIFI_MANAGER_H
