#include "akari_http.h"
#include <stdio.h>
#include <signal.h>

void handle_signal(int sig) {
    (void)sig;
    printf("\nShutting down Akari...\n");
    akari_stop();
}

void handle_home(akari_context* ctx) {
    akari_res_file(ctx, "index.html");
}

int main() {
    signal(SIGINT, handle_signal);
    printf("🏮 Akari starting on Linux (Port 8080)...\n");
    
    AKARI_GET("/", handle_home);
    
    akari_http_start(8080);
    return 0;
}
