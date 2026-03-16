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
"idf_component_register(SRCS \"main.c\" \"wifi.c\"\n"
"                    INCLUDE_DIRS \".\"\n"
"                    EMBED_FILES \"index.html\")\n";

const char* ESP_COMPONENT_YML_TEMPLATE = 
"dependencies:\n"
"  akari:\n"
"    git: https://github.com/creedpetitt/akari.git\n";

const char* ESP_MAIN_C_TEMPLATE = 
"#include \"akari_http.h\"\n"
"#include \"esp_log.h\"\n"
"#include \"wifi.h\"\n"
"\n"
"static const char *TAG = \"akari_esp32\";\n"
"\n"
"extern const uint8_t index_html_start[] asm(\"_binary_index_html_start\");\n"
"extern const uint8_t index_html_end[]   asm(\"_binary_index_html_end\");\n"
"\n"
"void handle_home(akari_context* ctx) {\n"
"    size_t len = index_html_end - index_html_start;\n"
"    akari_res_flash(ctx, 200, \"text/html\", index_html_start, len);\n"
"}\n"
"\n"
"void app_main(void) {\n"
"    ESP_LOGI(TAG, \"🏮 Starting Akari on ESP32...\");\n"
"    wifi_init();\n"
"    \n"
"    AKARI_GET(\"/\", handle_home);\n"
"    akari_http_start(80);\n"
"}\n";

const char* ESP_WIFI_C_TEMPLATE = 
"#include \"wifi.h\"\n"
"#include <stdio.h>\n"
"#include \"esp_wifi.h\"\n"
"#include \"esp_event.h\"\n"
"#include \"nvs_flash.h\"\n"
"#include \"esp_netif.h\"\n"
"#include \"esp_log.h\"\n"
"#include \"secrets.h\"\n"
"\n"
"static void wifi_event_handler(void* arg, esp_event_base_t event_base,\n"
"                             int32_t event_id, void* event_data) {\n"
"    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {\n"
"        esp_wifi_connect();\n"
"        printf(\"Connecting to WiFi...\\n\");\n"
"    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {\n"
"        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;\n"
"        printf(\"Connected! IP Address: \" IPSTR \"\\n\", IP2STR(&event->ip_info.ip));\n"
"    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {\n"
"        esp_wifi_connect();\n"
"        printf(\"Retrying connection...\\n\");\n"
"    }\n"
"}\n"
"\n"
"void wifi_init(void) {\n"
"    esp_err_t ret = nvs_flash_init();\n"
"    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {\n"
"        ESP_ERROR_CHECK(nvs_flash_erase());\n"
"        ret = nvs_flash_init();\n"
"    }\n"
"    ESP_ERROR_CHECK(ret);\n"
"\n"
"    ESP_ERROR_CHECK(esp_netif_init());\n"
"    ESP_ERROR_CHECK(esp_event_loop_create_default());\n"
"    esp_netif_create_default_wifi_sta();\n"
"\n"
"    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();\n"
"    ESP_ERROR_CHECK(esp_wifi_init(&cfg));\n"
"\n"
"    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));\n"
"    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));\n"
"\n"
"    wifi_config_t wifi_config = {\n"
"            .sta = {\n"
"                .ssid = WIFI_SSID,\n"
"                .password = WIFI_PASS,\n"
"            },\n"
"    };\n"
"\n"
"    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));\n"
"    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));\n"
"    ESP_ERROR_CHECK(esp_wifi_start());\n"
"}\n";

const char* ESP_WIFI_H_TEMPLATE = 
"#ifndef WIFI_H\n"
"#define WIFI_H\n"
"\n"
"void wifi_init(void);\n"
"\n"
"#endif // WIFI_H\n";

const char* ESP_SECRETS_H_TEMPLATE = 
"#ifndef SECRETS_H\n"
"#define SECRETS_H\n"
"\n"
"#define WIFI_SSID \"YOUR_WIFI_SSID\"\n"
"#define WIFI_PASS \"YOUR_WIFI_PASSWORD\"\n"
"\n"
"#endif // SECRETS_H\n";

const char* ESP_README_MD_TEMPLATE = 
"# Akari ESP32 Project\n"
"\n"
"## Quick Start\n"
"\n"
"1. Configure Wi-Fi Credentials:\n"
"   Open `main/secrets.h` and enter your Wi-Fi SSID and Password.\n"
"\n"
"2. Build and Flash:\n"
"   Connect your ESP32 via USB and run the following command to build, flash, and open the serial monitor:\n"
"   ```bash\n"
"   idf.py build flash monitor\n"
"   ```\n"
"\n"
"   *(Press `Ctrl+]` to exit the monitor).*\n"
"\n"
"## Troubleshooting\n"
"* Ensure you have the ESP-IDF environment activated (`get_idf` or `. export.sh`).\n"
"* Check if the correct serial port is selected automatically, or specify it with `-p /dev/ttyUSB0`.\n";



#endif
