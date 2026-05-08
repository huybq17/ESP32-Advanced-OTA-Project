#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H


#define WIFI_SSID      "Phong 102"
#define WIFI_PASS      "leanh102"

// Khởi tạo và kết nối Wi-Fi. Module này hoạt động độc lập và 
// sẽ phát (publish) sự kiện thông qua esp_event_loop.
void wifi_manager_init(void);

#endif // WIFI_MANAGER_H
