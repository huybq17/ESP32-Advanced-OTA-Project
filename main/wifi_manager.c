#include "wifi_manager.h"
#include "app_events.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI_MODULE";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        // PUBLISH: Phát tín hiệu rớt mạng ra toàn hệ thống
        esp_event_post(APP_EVENTS, APP_EVENT_WIFI_DISCONNECTED, NULL, 0, portMAX_DELAY);
        ESP_LOGI(TAG, "Kết nối lại tới Access Point");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Bắt được IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // PUBLISH: Phát tín hiệu ĐÃ CÓ MẠNG (OTA_Module sẽ hứng lấy cái này để chạy)
        esp_event_post(APP_EVENTS, APP_EVENT_WIFI_CONNECTED, NULL, 0, portMAX_DELAY);
    }
}

void wifi_manager_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Lắng nghe các thay đổi Wifi/IP Default từ ESP-IDF sau đó Emit vào APP_EVENTS
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "Khởi tạo thành công Wifi STA Manager.");
}
