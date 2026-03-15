# Akari Examples

This directory contains reference implementations to help you get started with the Akari framework.

- **[basic_linux](basic_linux/)**: The standard implementation for Linux/macOS, demonstrating how to compile Akari from its raw source files using a standard Makefile. It achieved 230,000 req/sec in benchmarking.
- **[basic_esp32](basic_esp32/)**: A complete ESP-IDF component implementation demonstrating how to run Akari on an ESP32 chip alongside the standard Wi-Fi boilerplate.

## Recommendation for New Projects
While these examples are great for absolute control, we strongly recommend using the official CLI tool to bootstrap new projects:

```bash
# Generate a standard C project with a self-contained header library
akari init my_app

# Generate a complete ESP32 project with all Wi-Fi and CMake boilerplate
akari init my_esp_app --esp32
```
