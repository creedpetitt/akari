# Akari Embedded Deployment

Akari is designed from the ground up to be "Tunable" for real-time operating systems (RTOS) and bare-metal ARM Cortex-M microcontrollers.

## Memory Tuning

Unlike standard servers, Akari's RAM usage is controlled by simple `#define` macros. You can set these in your `main.c` or via your compiler flags (e.g., `-DAKARI_MAX_CONNECTIONS=4`).

### `AKARI_MAX_CONNECTIONS`
**Default: 8**
Controls the number of simultaneous TCP connections in the pre-allocated pool.
*   **Linux/High-Load:** Set to 128-1024.
*   **ESP32/IoT:** Set to 4-8.
*   **Low-Memory (LPC1768):** Set to 2-4 to stay under 64KB RAM.

### `AKARI_MAX_ROUTES`
**Default: 16**
The maximum number of `AKARI_GET` or `AKARI_POST` handlers you can register. 
*   Increasing this only costs 192 bytes of RAM per 16 routes.

### `AKARI_RES_BUF_SIZE`
**Default: 512**
The size of the stack buffer used by `akari_printf`. This buffer is allocated **on the stack** within the `akari_context` during a request. It does not stay in RAM permanently.

---

## Hardware Support Guides

### ESP32 (ESP-IDF)
To run Akari on an ESP32:
1.  Copy `akari.h` to your `main/` directory.
2.  Enable POSIX sockets in `sdkconfig` (usually enabled by default).
3.  Include `akari.h` and use the standard `akari_http_start` function.
4.  Akari will automatically detect the ESP32 and use the `poll()` engine.

### ARM Cortex-M (Mbed OS)
On older Mbed chips like the LPC1768:
1.  Ensure you have enough stack space (8KB recommended).
2.  Set `AKARI_MAX_CONNECTIONS` to 4.
3.  Akari will run as a high-performance web controller for your sensors.

### Bare-Metal & ST-HAL
If you are running without an OS, you simply need to provide a standard POSIX-compliant socket layer (like **lwIP**). As long as Akari can call `socket()`, `bind()`, and `poll()`, it will run.
