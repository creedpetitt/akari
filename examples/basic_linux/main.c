#include "akari_http.h"
#include <stdio.h>
#include <signal.h>

void handle_signal(int sig) {
    (void)sig;
    printf("\nShutting down Akari...\n");
    akari_stop();
}

static const char BENCHMARK_PAYLOAD[] = "Hello from Akari!";

void handle_benchmark(akari_context* ctx) {
    size_t len = sizeof(BENCHMARK_PAYLOAD) - 1;
    akari_res_data(ctx, 200, "text/plain", BENCHMARK_PAYLOAD, len);
}

int main() {
    signal(SIGINT, handle_signal);
    printf("🏮 Akari starting on Linux (Benchmark Mode)...\n");
    
    AKARI_GET("/", handle_benchmark);
    
    akari_http_start(8080);
    return 0;
}
