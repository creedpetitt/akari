#ifndef AKARI_TEMPLATES_H
#define AKARI_TEMPLATES_H

const char* INDEX_HTML_TEMPLATE = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <title>Akari Server</title>\n"
"    <style>\n"
"        body { font-family: sans-serif; text-align: center; padding: 50px; }\n"
"        h1 { color: #ff4500; }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <h1>🏮 Akari Web Server</h1>\n"
"    <p>Your zero-allocation C server is running perfectly.</p>\n"
"</body>\n"
"</html>\n";

const char* MAIN_C_TEMPLATE = 
"#define AKARI_IMPLEMENTATION\n"
"#include \"akari.h\"\n"
"#include <stdio.h>\n"
"#include <signal.h>\n"
"\n"
"void handle_signal(int sig) {\n"
"    (void)sig;\n"
"    printf(\"\\nShutting down Akari...\\n\");\n"
"    akari_stop();\n"
"}\n"
"\n"
"void handle_home(akari_context* ctx) {\n"
"    akari_res_file(ctx, \"index.html\");\n"
"}\n"
"\n"
"int main() {\n"
"    signal(SIGINT, handle_signal);\n"
"    printf(\"🏮 Akari starting on port 8080...\\n\");\n"
"    \n"
"    AKARI_GET(\"/\", handle_home);\n"
"    \n"
"    akari_http_start(8080);\n"
"    return 0;\n"
"}\n";

const char* MAKEFILE_TEMPLATE = 
"all:\n"
"	gcc -O2 main.c -o app\n"
"clean:\n"
"	rm -f app\n";

const char* ESP_CMAKE_PROJECT_TEMPLATE = 
"cmake_minimum_required(VERSION 3.16)\n"
"include($ENV{IDF_PATH}/tools/cmake/project.cmake)\n"
"project(%s)\n";

const char* ESP_CMAKE_MAIN_TEMPLATE = 
"idf_component_register(SRCS \"main.c\"\n"
"                    INCLUDE_DIRS \".\"\n"
"                    EMBED_FILES \"index.html\")\n";

const char* ESP_COMPONENT_YML_TEMPLATE = 
"dependencies:\n"
"  akari:\n"
"    git: https://github.com/creedpetitt/akari.git\n";

const char* ESP_MAIN_C_TEMPLATE = 
"#include \"akari_http.h\"\n"
"#include \"esp_wifi.h\"\n"
"#include \"esp_event.h\"\n"
"#include \"nvs_flash.h\"\n"
"#include \"esp_netif.h\"\n"
"#include \"esp_log.h\"\n"
"\n"
"static const char *TAG = \"akari_esp32\";\n"
"\n"
"extern const uint8_t index_html_start[] asm(\"_binary_index_html_start\");\n"
"extern const uint8_t index_html_end[]   asm(\"_binary_index_html_end\");\n"
"\n"
"void handle_home(akari_context* ctx) {\n"
"    size_t len = index_html_end - index_html_start;\n"
"    akari_res_data(ctx, 200, \"text/html\", index_html_start, len);\n"
"}\n"
"\n"
"void wifi_init() {\n"
"    ESP_ERROR_CHECK(nvs_flash_init());\n"
"    ESP_ERROR_CHECK(esp_netif_init());\n"
"    ESP_ERROR_CHECK(esp_event_loop_create_default());\n"
"    esp_netif_create_default_wifi_sta();\n"
"    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();\n"
"    ESP_ERROR_CHECK(esp_wifi_init(&cfg));\n"
"    \n"
"    wifi_config_t wifi_config = {\n"
"        .sta = {\n"
"            .ssid = \"YOUR_WIFI_SSID\",\n"
"            .password = \"YOUR_WIFI_PASSWORD\",\n"
"        },\n"
"    };\n"
"    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));\n"
"    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));\n"
"    ESP_ERROR_CHECK(esp_wifi_start());\n"
"    ESP_ERROR_CHECK(esp_wifi_connect());\n"
"}\n"
"\n"
"void app_main(void) {\n"
"    ESP_LOGI(TAG, \"🏮 Starting Akari on ESP32...\");\n"
"    wifi_init();\n"
"    \n"
"    AKARI_GET(\"/\", handle_home);\n"
"    akari_http_start(80);\n"
"}\n";

#endif
