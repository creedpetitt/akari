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
Controls the number of simultaneous TCP connections in the pre-allocated pool. Each connection costs approximately ~5KB of static RAM (depending heavily on `AKARI_REQ_BUF_SIZE` and `AKARI_RES_BUF_SIZE`).

#### `AKARI_MAX_ROUTES`
**Default: 16**
The maximum number of `AKARI_GET` or `AKARI_POST` handlers. Increasing this is extremely cheap (~192 bytes per 16 routes).

#### `AKARI_RES_BUF_SIZE`
**Default: 512**
The size of the persistent transmission buffer residing in each connection object. It dictates the maximum size of dynamic JSON/HTML responses built via `akari_printf`. Static files streamed via `akari_res_file` bypass this limit completely by reading directly from disk in 1KB chunks.

---

## Hardware Support Guides

### ESP32 (ESP-IDF)
To run Akari on an ESP32, you should link it as an official component:
1.  Create an `idf_component.yml` file in your `main/` directory:
    ```yaml
    dependencies:
      akari:
        git: https://github.com/creedpetitt/akari.git
    ```
2.  Include `"akari_http.h"` in your `main.c` and use the standard `akari_http_start` function.
3.  Akari will automatically detect the ESP32, include the correct `lwip` definitions, and use the `poll()` engine.

**Recommendation:** For an instant boilerplate project with complete Wi-Fi setup logic, we strongly recommend referencing the [examples/basic_esp32](../examples/basic_esp32) directory or using the `akari init <name> --esp32` CLI tool!

### ARM Cortex-M (Bare-Metal)
On older ARM Cortex chips like the STM32:
1.  Ensure you have enough stack space (8KB recommended).
2.  Set `AKARI_MAX_CONNECTIONS` to 4.
3.  Akari will run as a high-performance web controller for your sensors.

### Bare-Metal & ST-HAL
If you are running without an OS, you simply need to provide a standard POSIX-compliant socket layer (like **lwIP**). As long as Akari can call `socket()`, `bind()`, and `poll()`, it will run.
