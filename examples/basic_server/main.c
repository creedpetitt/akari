#include "akari_http.h"
#include <stdio.h>
#include <string.h>

void handle_home(akari_context* ctx) {
    akari_res_file(ctx, "index.html");
}

void handle_api(akari_context* ctx) {
    printf("Someone visited the API!\n");
    akari_res_send(ctx, 200, "application/json", "{\"status\": \"online\", \"chip\": \"LPC1768\"}");
}

void handle_user(akari_context* ctx) {
    int id = akari_param_to_int(ctx, "id");
    const char* action = akari_query_str(ctx, "action", "view");

    akari_printf(ctx, "User ID: %d\nAction: %s\n", id, action);
    akari_send(ctx, 200, "text/plain");
}

void handle_post_data(akari_context* ctx) {
    size_t name_len;
    const char* name = akari_json_get_string(ctx, "name", &name_len);
    int age = akari_json_get_int(ctx, "age");
    int is_admin = akari_json_get_bool(ctx, "is_admin");

    char response[256];
    if (name) {
        snprintf(response, sizeof(response), 
                 "Hello %.*s! You are %d years old. Admin: %s", 
                 (int)name_len, name, age, is_admin ? "Yes" : "No");
    } else {
        snprintf(response, sizeof(response), "Error: 'name' missing in JSON.");
    }

    akari_res_json(ctx, 200, response);
}

#include <signal.h>

void handle_signal(int sig) {
    (void)sig;
    printf("\nShutting down Akari...\n");
    akari_stop();
}

void read_sensor() {
    printf("[TIMER] Reading hardware sensors...\n");
}

int main() {
    signal(SIGINT, handle_signal);
    printf("Starting Akari Web Server on port 8080...\n");

    // Start background task
    akari_add_timer(read_sensor, 2000);

    AKARI_GET("/", handle_home);
    AKARI_GET("/api", handle_api);
    AKARI_GET("/user/:id", handle_user);
    AKARI_POST("/data", handle_post_data);

    akari_http_start(8080);

    return 0;
}
