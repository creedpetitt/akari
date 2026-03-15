# Akari ESP32 Example

This directory contains a complete, ready-to-run ESP32 project using the Akari web framework.

## Quick Start

1. **Configure Wi-Fi Credentials:**
   Open `main/secrets.h` and enter your Wi-Fi SSID and Password.

2. **Build and Flash:**
   Connect your ESP32 via USB and run the following command to build, flash, and open the serial monitor:
   ```bash
   idf.py build flash monitor
   ```

   *(Press `Ctrl+]` to exit the monitor).*

## Why is `wifi.c` included here?
Akari is a pure HTTP framework. It doesn't dictate how your ESP32 connects to the internet (Wi-Fi, Ethernet, Cellular, etc). 
This `wifi.c` file is provided simply as a reference for handling standard ESP32 Wi-Fi boilerplate. Feel free to copy/paste it into your own custom projects!
