#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h" // Thư viện cho cấu hình HTTP Client (thuộc Bước 3)
#include "esp_https_ota.h"   // Thư viện xử lý việc cập nhật OTA (thuộc Bước 3)

// Thay bằng Tên/Mật khẩu Wi-Fi của bạn
#define WIFI_SSID      "P204"
#define WIFI_PASS      "P204@123"

// Thiết lập URL chỉ tới file update.bin trên Local Server vừa tạo ở Bước 2
#define OTA_URL        "http://192.168.24.102:8000/update.bin"

static const char *TAG = "OTA_PROJECT";
static bool is_ota_started = false; 

// Phần 3.1: Hàm cốt lõi thực thi quy trình OTA
void ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Đang bắt đầu tiến trình OTA...");

    // Cấu hình HTTP Client để ESP32 biết phải tải file từ đâu
    esp_http_client_config_t config = {
        .url = OTA_URL,
        // Vì chạy thử bằng HTTP (không bảo mật), ta cần bỏ qua xác thực ở bước này
        // (Đây là thủ thuật test, bước 4 ta sẽ áp dụng bảo mật thật)
        .use_global_ca_store = false,
        .crt_bundle_attach = NULL,
    };

    // Đưa cấu hình HTTP vào cấu hình OTA
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    ESP_LOGI(TAG, "Đang kéo Firmware mới từ: %s", config.url);
    
    // Gọi hàm phép thuật thay đổi số phận của bộ nhớ Flash
    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Cập nhật OTA Thành công! Hệ thống chuẩn bị khởi động lại sau 3s...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart(); // Lệnh khởi động lại ESP32
    } else {
        // In ra mã lỗi nếu cập nhật thất bại
        ESP_LOGE(TAG, "OTA Thất bại. Mã lỗi (esp_err_t): %s", esp_err_to_name(ret));
    }

    // Kết thúc task khi hoàn thành
    vTaskDelete(NULL);
}

// Hàm xử lý sự kiện Wi-Fi (Event Handler)
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnect Wi-Fi, trying connecting again...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Phần 3.2: Khi có được IP mạng thì sinh ra tiến trình OTA ngầm
        // static bool is_ota_started = false; 
        if (!is_ota_started) {
            ESP_LOGI(TAG, "Chuẩn bị khởi chạy luồng OTA sau 5 giây (chờ kết nối hoàn toàn ổn định)");
            vTaskDelay(pdMS_TO_TICKS(5000));
            xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
            is_ota_started = true; // Chỉ chạy gọi OTA đúng 1 lần sau khi kết nối
        }

    }
}

// Khởi tạo Wi-Fi (Chế độ Station)
void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi station initialization complete....");
}

void app_main(void)
{
    // 1. Khởi tạo NVS Flash (Rất quan trọng cho Wi-Fi & OTA)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Gọi hàm nạp cấu hình và kết nối Wi-Fi
    wifi_init_sta();

    // Vòng lặp chính vĩnh viễn giúp ESP32 tiếp tục sống
    while (1) {
        ESP_LOGI(TAG, "System running... Waiting for OTA updates or other tasks.");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}