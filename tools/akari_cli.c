#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "templates.h"
#include "akari_lib.h"

void write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

void write_binary_file(const char* path, const unsigned char* data, unsigned int len) {
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(data, 1, len, f);
        fclose(f);
    }
}

void print_help() {
    printf("Usage: akari <command> [args]\n\n");
    printf("Commands:\n");
    printf("  init <name> [--esp32]    Initialize a new project\n");
    printf("  version                  Show Akari version\n");
    printf("  --help                   Display this help\n");
}

int do_init(const char* name, int is_esp32) {
    printf("🏮 Initializing %s project '%s'...\n", is_esp32 ? "ESP32" : "standard", name);
    
    if (mkdir(name, 0755) == -1) {
        perror("mkdir failed");
        return 1;
    }

    if (chdir(name) == -1) {
        perror("chdir failed");
        return 1;
    }

    if (is_esp32) {
        char cmake_root[256];
        snprintf(cmake_root, sizeof(cmake_root), ESP_CMAKE_PROJECT_TEMPLATE, name);
        write_file("CMakeLists.txt", cmake_root);
        
        if (mkdir("main", 0755) == -1) {
            perror("mkdir main failed");
            return 1;
        }
        
        write_file("main/CMakeLists.txt", ESP_CMAKE_MAIN_TEMPLATE);
        write_file("main/idf_component.yml", ESP_COMPONENT_YML_TEMPLATE);
        write_file("main/main.c", ESP_MAIN_C_TEMPLATE);
        write_file("main/wifi.c", ESP_WIFI_C_TEMPLATE);
        write_file("main/wifi.h", ESP_WIFI_H_TEMPLATE);
        write_file("main/secrets.h", ESP_SECRETS_H_TEMPLATE);
        write_file("main/index.html", INDEX_HTML_TEMPLATE);
        write_file("README.md", ESP_README_MD_TEMPLATE);
    } else {
        write_file("index.html", INDEX_HTML_TEMPLATE);
        write_file("main.c", MAIN_C_TEMPLATE);
        write_file("Makefile", MAKEFILE_TEMPLATE);
        write_binary_file("akari.h", AKARI_LIB_DATA, AKARI_LIB_LEN);
    }

    printf("\nSuccess! To start your server:\n");
    if (is_esp32) {
        printf("  cd %s && idf.py build\n", name);
    } else {
        printf("  cd %s && make && ./app\n", name);
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        if (argc < 3) {
            printf("Error: 'init' requires a project name.\n");
            return 1;
        }
        int is_esp32 = 0;
        if (argc >= 4 && strcmp(argv[2], "--esp32") == 0) {
            return do_init(argv[3], 1);
        } else if (argc >= 4 && strcmp(argv[3], "--esp32") == 0) {
            return do_init(argv[2], 1);
        }
        return do_init(argv[2], 0);
    } else if (strcmp(argv[1], "version") == 0) {
        printf("Akari 0.1.0\n");
        return 0;
    } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    printf("Unknown command: %s\n", argv[1]);
    print_help();
    return 1;
}
