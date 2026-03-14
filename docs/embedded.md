# Akari Embedded Deployment

Akari is designed to be compatible for real-time operating systems (RTOS) and bare-metal ARM Cortex-M microcontrollers.

## Hardware Tuning 

Akari can be configured for your specific hardware. Use the table below to configure your compiler flags (e.g., `-DAKARI_MAX_CONNECTIONS=4`).

| Hardware Profile | `AKARI_MAX_CONNECTIONS` | `AKARI_RES_BUF_SIZE` | Ideal Target |
| :--- | :---: | :---: | :--- |
| **Tiny Core** | 2 - 4 | 256 bytes | STM32F1, 64KB RAM |
| **IoT Standard** | 8 - 16 | 1 KB | ESP32, 520KB RAM |
| **Edge Gateway** | 64 - 128 | 4 KB | Raspberry Pi, BeagleBone |
| **High-Perf** | 1024+ | 8 KB+ | Linux Server / Benchmark |

### Tuning Macros

#### `AKARI_MAX_CONNECTIONS`
Controls the number of simultaneous TCP connections in the pre-allocated pool. Each connection costs approximately 4KB of static RAM (depending on `AKARI_BUF_SIZE`).

#### `AKARI_MAX_ROUTES`
**Default: 16**
The maximum number of `AKARI_GET` or `AKARI_POST` handlers. Increasing this is extremely cheap (~192 bytes per 16 routes).

#### `AKARI_RES_BUF_SIZE`
**Default: 512**
The size of the stack buffer used by `akari_printf`. This lives on the stack *only* during the request.

---

## Hardware Support Guides

### ESP32 (ESP-IDF)
To run Akari on an ESP32:
1.  Copy `akari.h` to your `main/` directory.
2.  Enable POSIX sockets in `sdkconfig` (usually enabled by default).
3.  Include `akari.h` and use the standard `akari_http_start` function.
4.  Akari will automatically detect the ESP32 and use the `poll()` engine.

### ARM Cortex-M (Bare-Metal)
On older ARM Cortex chips like the STM32:
1.  Ensure you have enough stack space (8KB recommended).
2.  Set `AKARI_MAX_CONNECTIONS` to 4.
3.  Akari will run as a high-performance web controller for your sensors.

### Bare-Metal & ST-HAL
If you are running without an OS, you simply need to provide a standard POSIX-compliant socket layer (like **lwIP**). As long as Akari can call `socket()`, `bind()`, and `poll()`, it will run.
