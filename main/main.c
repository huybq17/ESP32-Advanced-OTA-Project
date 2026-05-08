#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"

#include "app_events.h"
#include "app_init.h"

static const char *TAG = "APP_MAIN";

// HIGH-LEVEL SUBSCRIBE: Main listens to OTA signals
static void main_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == APP_EVENTS) {
        if (event_id == APP_EVENT_WIFI_DISCONNECTED) {
            ESP_LOGW(TAG, "MAIN: Wi-Fi disconnected --> Suspend LED/Buzzer.");
        } else if (event_id == APP_EVENT_OTA_STARTED) {
            ESP_LOGI(TAG, "MAIN: OTA Started. Blinking LED continuously...");
        } else if (event_id == APP_EVENT_OTA_FAILED) {
            ESP_LOGI(TAG, "MAIN: OTA Failed, disabling update LED mode.");
        }
    }
}

void app_main(void)
{
    // Initialize all core system tasks and sub-modules
    app_init_modules();

    // Register to listen to all signals from APP_EVENTS
    esp_event_handler_instance_register(APP_EVENTS, ESP_EVENT_ANY_ID, &main_event_handler, NULL, NULL);

    while (1) {
        ESP_LOGI(TAG, "Waiting other threads...");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
