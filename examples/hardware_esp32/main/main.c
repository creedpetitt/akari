#include "akari_http.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include <string.h>

static const char *TAG = "akari_esp32";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

// 1. Define Shared State
typedef enum { LED_OFF, LED_ON, LED_BLINK } led_mode_t;

typedef struct {
    gpio_num_t pin;
    volatile led_mode_t mode;
    volatile uint32_t delay_ms;
} led_control_t;

// Initialize our two LEDs to blink at 1000ms by default
led_control_t green_led = { GPIO_NUM_4, LED_BLINK, 1000 };
led_control_t red_led = { GPIO_NUM_5, LED_BLINK, 1000 };

// 2. The RTOS Task for LEDs
void led_task(void *pvParameter) {
    led_control_t *led = (led_control_t *)pvParameter;
    gpio_set_direction(led->pin, GPIO_MODE_OUTPUT);
    uint8_t toggle = 0;
    
    while(1) {
        if (led->mode == LED_ON) {
            gpio_set_level(led->pin, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } 
        else if (led->mode == LED_OFF) {
            gpio_set_level(led->pin, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS); 
        } 
        else if (led->mode == LED_BLINK) {
            toggle = !toggle;
            gpio_set_level(led->pin, toggle);
            uint32_t current_delay = led->delay_ms > 0 ? led->delay_ms : 100;
            vTaskDelay(current_delay / portTICK_PERIOD_MS);
        }
    }
}

// 3. Handlers
void handle_home(akari_context* ctx) {
    size_t len = index_html_end - index_html_start;
    akari_res_flash(ctx, 200, "text/html", index_html_start, len);
}

void handle_led_state(akari_context* ctx) {
    size_t c_len, s_len;
    const char* color = akari_get_path_param(ctx, "color", &c_len);
    const char* state = akari_get_path_param(ctx, "state", &s_len);

    led_control_t *target = NULL;
    if (strncmp(color, "red", c_len) == 0) target = &red_led;
    else if (strncmp(color, "green", c_len) == 0) target = &green_led;

    if (!target) {
        akari_res_send(ctx, 404, "text/plain", "Unknown color");
        return;
    }

    if (strncmp(state, "on", s_len) == 0) target->mode = LED_ON;
    else if (strncmp(state, "off", s_len) == 0) target->mode = LED_OFF;
    else if (strncmp(state, "blink", s_len) == 0) target->mode = LED_BLINK;
    else {
        akari_res_send(ctx, 400, "text/plain", "Unknown state");
        return;
    }

    akari_printf(ctx, "{\"status\": \"success\"}");
    akari_send(ctx, 200, "application/json");
}

void handle_led_rate(akari_context* ctx) {
    size_t c_len;
    const char* color = akari_get_path_param(ctx, "color", &c_len);
    int ms = akari_param_to_int(ctx, "ms"); 

    if (ms <= 0) {
        akari_res_send(ctx, 400, "text/plain", "Invalid rate");
        return;
    }

    led_control_t *target = NULL;
    if (strncmp(color, "red", c_len) == 0) target = &red_led;
    else if (strncmp(color, "green", c_len) == 0) target = &green_led;

    if (target) {
        target->delay_ms = ms;
        akari_printf(ctx, "{\"status\": \"success\", \"rate_ms\": %d}", ms);
        akari_send(ctx, 200, "application/json");
    } else {
        akari_res_send(ctx, 404, "text/plain", "Unknown color");
    }
}

// 4. Main
void app_main(void) {
    ESP_LOGI(TAG, "Starting Akari on ESP32...");
    wifi_init();
    
    // Start the LED background tasks
    xTaskCreate(&led_task, "red_task", 2048, &red_led, 5, NULL);
    xTaskCreate(&led_task, "grn_task", 2048, &green_led, 5, NULL);
    
    // Register the routes (Order matters!)
    AKARI_GET("/", handle_home);
    AKARI_GET("/api/led/:color/rate", handle_led_rate);     
    AKARI_GET("/api/led/:color/:state", handle_led_state);  
    
    akari_http_start(80);
}