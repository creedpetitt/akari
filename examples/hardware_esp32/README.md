# Akari ESP32 Project

## Quick Start

1. Configure Wi-Fi Credentials:
   Open `main/secrets.h` and enter your Wi-Fi SSID and Password.

2. Build and Flash:
   Connect your ESP32 via USB and run the following command to build, flash, and open the serial monitor:
   ```bash
   idf.py build flash monitor
   ```

   *(Press `Ctrl+]` to exit the monitor).*

## Troubleshooting
* Ensure you have the ESP-IDF environment activated (`get_idf` or `. export.sh`).
* Check if the correct serial port is selected automatically, or specify it with `-p /dev/ttyUSB0`.
