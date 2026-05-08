#include "app_init.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "app_events.h"
#include "wifi_manager.h"
#include "ota_manager.h"

static const char *TAG = "APP_INIT";

void app_init_modules(void)
{
    ESP_LOGI(TAG, "==== INITIALIZING CENTRAL MODULE ARCHITECTURE ==== ");

    // 1. INIT BASIC HARDWARE
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. INITIALIZE CENTRAL EVENT BUS
    // Must be initialized before all other modules
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. START INITIAL MODULES
    ota_manager_init();   // Setup OTA connection listener
    wifi_manager_init();  // After Wi-Fi connects -> Emit APP_WIFI_CONNECTED -> ESP-IDF handles the rest!

    // [FUTURE] -> You can add future module inits here: 
    // mqtt_manager_init();
    // sensor_manager_init();
}
