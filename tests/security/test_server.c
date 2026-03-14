#include "akari_http.h"
#include <stdio.h>
#include <signal.h>

void handle_api(akari_context* ctx) {
    akari_res_send(ctx, 200, "application/json", "{\"status\":\"ok\"}");
}

void handle_post(akari_context* ctx) {
    akari_printf(ctx, "got %zu bytes", ctx->body_len);
    akari_send(ctx, 200, "text/plain");
}

void handle_signal(int sig) {
    (void)sig;
    akari_stop();
}

int main(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    AKARI_GET("/api", handle_api);
    AKARI_POST("/data", handle_post);

    akari_http_start(9777);
    return 0;
}
