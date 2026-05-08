#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"

// MODULE INCLUDES
#include "app_events.h"
#include "wifi_manager.h"
#include "ota_manager.h"

static const char *TAG = "APP_MAIN";

// SUBSCRIBE CẤP CAO: Main tự nghe tín hiệu OTA để làm việc
static void main_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == APP_EVENTS) {
        if (event_id == APP_EVENT_WIFI_DISCONNECTED) {
            ESP_LOGW(TAG, "MAIN nghe Wi-Fi rớt --> Tạm hoãn hệ thống đèn còi.");
        } else if (event_id == APP_EVENT_OTA_STARTED) {
            ESP_LOGI(TAG, "MAIN nghe báo OTA Chạy. Bật chớp tắt đèn led liên tục...");
        } else if (event_id == APP_EVENT_OTA_FAILED) {
            ESP_LOGI(TAG, "MAIN nghe khóc: OTA lỗi, tắt chế độ LED cập nhật.");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "==== KHỞI TẠO KIẾN TRÚC MODULE TRUNG TÂM ==== ");

    // 1. INIT BASIC HARDWARE
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. KHỞI TẠO CENTRAL EVENT BUS (Trạm phát sóng của ESP-IDF Event)
    // Phải nằm trước tất cả các module
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Đăng ký nghe toàn bộ tín hiệu của tần số APP_EVENTS báo về Main_Manager
    esp_event_handler_instance_register(APP_EVENTS, ESP_EVENT_ANY_ID, &main_event_handler, NULL, NULL);

    // 3. START MODULES
    // Các Module này hoàn toàn độc lập, nằm khác chỗ, chỉ chung 1 cái đường lưới EVENT BUS
    ota_manager_init();   // Cài cắm tai nghe tín hiệu kết nối
    wifi_manager_init();  // Sau khi Wifi bắt mạng xong -> Emit Event APP_WIFI_CONNECTED -> ESP-IDF Tự lo!

    // 4. MAIN THREAD RUNNING 
    while (1) {
        ESP_LOGI(TAG, "Waiting other threads...");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
