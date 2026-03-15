#include "akari_http.h"
#include "esp_log.h"
#include "wifi.h"

static const char *TAG = "akari_esp32";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

void handle_home(akari_context* ctx) {
    size_t len = index_html_end - index_html_start;
    akari_res_data(ctx, 200, "text/html", index_html_start, len);
}

void app_main(void) {
    ESP_LOGI(TAG, "🏮 Starting Akari on ESP32...");
    wifi_init();
    
    AKARI_GET("/", handle_home);
    akari_http_start(80);
}
