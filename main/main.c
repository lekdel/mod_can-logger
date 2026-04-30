#include "esp_log.h"
#include "wifi_sta.h"
#include "can_reader.h"
#include "http_api.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Init WiFi...");
    wifi_sta_init_and_connect();

    ESP_LOGI(TAG, "Init CAN reader...");
    can_reader_start();

    ESP_LOGI(TAG, "Init HTTP API...");
    http_api_start();
}
