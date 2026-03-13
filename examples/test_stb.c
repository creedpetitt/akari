#define AKARI_IMPLEMENTATION
#include "akari.h"

void handle_hello(akari_context* ctx) {
    akari_printf(ctx, "Hello from Single-Header Akari!");
    akari_send(ctx, 200, "text/plain");
}

int main() {
    AKARI_GET("/", handle_hello);
    akari_http_start(8080);
    return 0;
}
