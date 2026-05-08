#include "ota_manager.h"
#include "app_events.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "OTA_MODULE";
static bool is_ota_running = false;

static void ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Đường truyền OTA được kích hoạt! Tiến hành kiểm tra phiên bản...");
    
    // PUBLISH: Phát tín hiệu đang bắt đầu quá trình OTA
    esp_event_post(APP_EVENTS, APP_EVENT_OTA_STARTED, NULL, 0, portMAX_DELAY);

    esp_http_client_config_t config = {
        .url = OTA_URL,
        .use_global_ca_store = false,
        .crt_bundle_attach = NULL,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    ESP_LOGI(TAG, "Đang kết nối tới %s", config.url);
    
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Không thể bắt đầu OTA (Lỗi: %s)", esp_err_to_name(err));
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
        is_ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 1. Lấy thông tin Firmware (Image Description) trên Server
    esp_app_desc_t server_app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &server_app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Đọc Header Firmware trên máy chủ thất bại");
        esp_https_ota_abort(https_ota_handle);
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
        is_ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 2. Lấy thông tin Firmware hiện tại đang chạy trên ESP32
    esp_app_desc_t running_app_desc;
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_ota_get_partition_description(running_partition, &running_app_desc);

    ESP_LOGI(TAG, "-> Phiên bản đang chạy: %s", running_app_desc.version);
    ESP_LOGI(TAG, "-> Phiên bản trên Server: %s", server_app_desc.version);

    // 3. So sánh: Nếu giống nhau thì dừng lại không OTA nữa để tránh Bootloop
    if (strcmp(running_app_desc.version, server_app_desc.version) == 0) {
        ESP_LOGW(TAG, "Phiên bản Firmware giống nhau. Bỏ qua và hủy quá trình OTA.");
        esp_https_ota_abort(https_ota_handle); // Hủy download
        
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_SUCCESS, NULL, 0, portMAX_DELAY);
        is_ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 4. Nếu khác nhau: Tiến hành tải và Flash
    ESP_LOGI(TAG, "Tìm thấy phiên bản mới! Đang tải dữ liệu...");
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break; // Hoàn tất tiến trình Loop
        }
    }

    // 5. Kết luận trạng thái
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // Tải thất bại hoặc bị ngắt
        ESP_LOGE(TAG, "Tải không được toàn vẹn dữ liệu");
        esp_https_ota_abort(https_ota_handle);
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
    } else {
        err = esp_https_ota_finish(https_ota_handle); // Ghi chốt Flash
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Cập nhật %s thành công -> Khởi động lại sau 2s...", server_app_desc.version);
            esp_event_post(APP_EVENTS, APP_EVENT_OTA_SUCCESS, NULL, 0, portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Finish thất bại!");
            esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
        }
    }

    is_ota_running = false;
    vTaskDelete(NULL);
}

// SUBSCRIBE CỐT LÕI: Lắng nghe tín hiệu khi nhận được Wi-Fi
static void ota_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    // Bắt đúng tần số của APP_EVENTS là APP_EVENT_WIFI_CONNECTED
    if (event_base == APP_EVENTS && event_id == APP_EVENT_WIFI_CONNECTED) {
        ESP_LOGI(TAG, "Nghe thấy tín hiệu Wi-Fi (Subscribe) -> Xin hỏi khởi chạy OTA..");
        
        if (!is_ota_running) {
            is_ota_running = true;
            // Cho delay 3s đợi mạng ổn định trước khi bắn HTTP Request
            vTaskDelay(pdMS_TO_TICKS(3000));
            xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
        }
    }
}

void ota_manager_init(void)
{
    // Đăng ký nghe lén (Subscribe) tín hiệu trạm APP_EVENT_WIFI_CONNECTED
    esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_WIFI_CONNECTED, &ota_event_handler, NULL, NULL);
    
    ESP_LOGI(TAG, "Module OTA sẵn sàng nằm vùng.");
}
