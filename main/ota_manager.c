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
    ESP_LOGI(TAG, "OTA channel activated! Checking version...");
    
    // PUBLISH: Emit OTA starting signal
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

    ESP_LOGI(TAG, "Connecting to %s", config.url);
    
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot start OTA (Error: %s)", esp_err_to_name(err));
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
        is_ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 1. Fetch Firmware info (Image Description) from Server
    esp_app_desc_t server_app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &server_app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read Firmware Header on server");
        esp_https_ota_abort(https_ota_handle);
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
        is_ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 2. Fetch currently running Firmware info on ESP32
    esp_app_desc_t running_app_desc;
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_ota_get_partition_description(running_partition, &running_app_desc);

    ESP_LOGI(TAG, "-> Running version: %s", running_app_desc.version);
    ESP_LOGI(TAG, "-> Server version: %s", server_app_desc.version);

    // 3. Compare: If identical, stop OTA to prevent bootloop
    if (strcmp(running_app_desc.version, server_app_desc.version) == 0) {
        ESP_LOGW(TAG, "Firmware versions are identical. Skipping and aborting OTA process.");
        esp_https_ota_abort(https_ota_handle); // Abort download
        
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_SUCCESS, NULL, 0, portMAX_DELAY);
        is_ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 4. If different: Proceed with download and Flash
    ESP_LOGI(TAG, "New version found! Downloading data...");
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break; // Finished Loop process
        }
    }

    // 5. Conclude status
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // Download failed or interrupted
        ESP_LOGE(TAG, "Data download incomplete / corrupted");
        esp_https_ota_abort(https_ota_handle);
        esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
    } else {
        err = esp_https_ota_finish(https_ota_handle); // Finalize Flash
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Updated to %s successfully -> Restarting in 2s...", server_app_desc.version);
            esp_event_post(APP_EVENTS, APP_EVENT_OTA_SUCCESS, NULL, 0, portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Finish failed!");
            esp_event_post(APP_EVENTS, APP_EVENT_OTA_FAILED, NULL, 0, portMAX_DELAY);
        }
    }

    is_ota_running = false;
    vTaskDelete(NULL);
}

// CORE SUBSCRIBE: Listen for Wi-Fi received signal
static void ota_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    // Catch the exact APP_EVENTS frequency: APP_EVENT_WIFI_CONNECTED
    if (event_base == APP_EVENTS && event_id == APP_EVENT_WIFI_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi signal received (Subscribe) -> Requesting to start OTA..");
        
        if (!is_ota_running) {
            is_ota_running = true;
            // Delay 3s to let network stabilize before sending HTTP Request
            vTaskDelay(pdMS_TO_TICKS(3000));
            xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
        }
    }
}

void ota_manager_init(void)
{
    // Subscribe to APP_EVENT_WIFI_CONNECTED signal
    esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_WIFI_CONNECTED, &ota_event_handler, NULL, NULL);
    
    ESP_LOGI(TAG, "OTA Module ready and standing by.");
}
