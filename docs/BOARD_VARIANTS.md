# Board Variant Guide

This project supports multiple ESP32 boards with different LoRa radio modules. Select the appropriate variant in `platformio.ini` or PlatformIO IDE.

## Quick Start

### In VS Code with PlatformIO IDE

1. Open the PlatformIO sidebar (click the PlatformIO icon)
2. Hover over **"esp32dev"** (or current environment name)
3. Click the **"Switch Environment"** button or right-click → **"Set as Default"**
4. Select your board from the list (e.g., `ttgo-lora32-v2`)
5. Build and upload as normal

### In `platformio.ini` (Manual Selection)

Change the default environment by setting `default_envs`:

```ini
[platformio]
default_envs = heltec-wifi-lora-32-v3
```

Or use command line:

```bash
pio run -e heltec-wifi-lora-32-v3 -t upload
```

## Available Board Variants

### Generic ESP32 DevKit (Multiple Radio Options)

| Environment | Radio Module | Frequency | Power Efficiency | Notes |
|---|---|---|---|---|
| **`esp32dev-sx1262`** | SX1262 | 868 MHz | ⭐⭐⭐⭐⭐ | **Recommended** - New, efficient, best range |
| **`esp32dev-sx1276`** | SX1276 (RFM95W) | 868 MHz | ⭐⭐⭐ | Popular choice, widely available |
| **`esp32dev-cc1101`** | CC1101 | 868 MHz | ⭐⭐⭐⭐ | Alternative option, lower power |
| **`esp32dev-lr1121`** | LR1121 | 868 MHz | ⭐⭐⭐⭐⭐ | Latest LoRa chip, future-proof |

**Default GPIO Pinout:**
- **CS (Chip Select)**: GPIO 27
- **IRQ (Interrupt)**: GPIO 21
- **BUSY/GPIO**: GPIO 33
- **RST (Reset)**: GPIO 32
- **SPI**: SCK=18, MOSI=23, MISO=19

See [**Radio Modules Guide**](RADIO_MODULES.md) for detailed comparison and custom pinning options.

### Dedicated Board Variants

| Environment | Board | Radio | Frequency | Notes |
|---|---|---|---|---|
| `ttgo-lora32-v1` | LILYGO TTGO LoRa32 V1 | SX1276 | 868 MHz | First generation, integrated antenna |
| `ttgo-lora32-v2` | LILYGO TTGO LoRa32 V2 | SX1276 | 868 MHz | Popular choice, excellent value |
| `ttgo-lora32-v21` | LILYGO TTGO LoRa32 V2.1 | SX1276 | 868 MHz | Improved V2 revision |
| `heltec-wireless-stick` | Heltec Wireless Stick | SX1276 | 868 MHz | Compact form factor |
| `heltec-wireless-stick-v3` | Heltec Wireless Stick V3 | SX1262 | 868 MHz | ESP32-S3, excellent range, compact |
| `heltec-wireless-stick-lite` | Heltec Wireless Stick Lite V3 | SX1262 | 868 MHz | Lighter version of Stick V3 |
| `heltec-wifi-lora-32-v2` | Heltec WiFi LoRa 32 V2 | SX1276 | 868 MHz | Larger board, built-in OLED display |
| `heltec-wifi-lora-32-v3` | Heltec WiFi LoRa 32 V3 | SX1262 | 868 MHz | Latest Heltec flagship with display |
| `heltec-wifi-lora-32-v4` | Heltec WiFi LoRa 32 V4 | SX1262 | 868 MHz | Newest Heltec version, improved design |
| `lilygo-t3-s3-sx1262` | LilyGo T3-S3 | SX1262 | 868 MHz | ESP32-S3 processor, excellent performance |
| `lilygo-t3-s3-sx1276` | LilyGo T3-S3 | SX1276 | 868 MHz | Same board, different radio option |
| `lilygo-t3-s3-lr1121` | LilyGo T3-S3 | LR1121 | 868 MHz | Latest technology on T3-S3 platform |
| `seeed-xiao-esp32s3` | Seeed XIAO ESP32S3 | SX1262 | 868 MHz | Tiny form factor, Wio-SX1262 module |
| `esp32s3-dev` | Generic ESP32-S3 DevKit | SX1276 | 868 MHz | Generic S3 fallback variant |

## Hardware Setup by Variant

### LILYGO TTGO LoRa32 V2 / V2.1
- **Integrated antenna** for 868 MHz
- **Display**: 128×64 OLED
- **Power**: USB-C or battery connector
- **Pins**: Pre-configured (CS=18, IRQ=26, RST=14, GPIO=33)

### Heltec Wireless Stick V3 / WiFi LoRa 32 V3 / V4
- **Integrated antenna** for 868 MHz
- **Display**: Optional OLED on some models
- **Power**: USB-C
- **Radio**: SX1262 (better range than SX1276)
- **Pins**: Pre-configured

### Adafruit Feather ESP32 + RFM95W FeatherWing
- **Radio Module**: RFM95W FeatherWing (bought separately)
- **Wiring Required**:
  - A (RST) → GPIO 27
  - B (DIO1) → GPIO 33
  - D (DIO0) → GPIO 32
  - E (CS) → GPIO 14
  - SPI: MOSI=23, MISO=19, SCK=18
- **Antenna**: External SMA connector recommended

### LilyGo T3-S3
- **Integrated antenna** for 868 MHz
- **ESP32-S3** processor (more powerful)
- **Display**: Optional
- **Radio**: SX1262 or SX1276 (choose variant)

### M5Stack Core2 + LoRa868 Module
- **Main Board**: M5Stack Core2
- **Module**: M5Stack Module LoRa868 (connects via socket)
- **Display**: 2" IPS LCD
- **Pins**: Pre-configured for module socket

## Adding Custom Board

If your board isn't listed:

1. **Identify the radio chip** (SX1276, SX1262, LR1121, or CC1101)
2. **Create a new environment** in `platformio.ini`:
   ```ini
   [env:my-custom-board]
   platform = espressif32
   board = <your_board_name>
   framework = arduino
   monitor_speed = 115200
   lib_deps =
       https://github.com/matthias-bs/BresserWeatherSensorReceiver.git
       jgromes/RadioLib
       tzapu/WiFiManager
       adafruit/Adafruit BMP280 Library
   build_flags =
       -D ARDUINO_MY_CUSTOM_BOARD
   ```

3. **Configure GPIO pins** in `src/WeatherSensorCfg.h`:
   ```cpp
   #elif defined(ARDUINO_MY_CUSTOM_BOARD)
       #pragma message("Custom board defined")
       #define USE_SX1276
       #define PIN_RECEIVER_CS   10
       #define PIN_RECEIVER_IRQ  21
       #define PIN_RECEIVER_GPIO 8
       #define PIN_RECEIVER_RST  9
   ```

4. **Build and upload**:
   ```bash
   pio run -e my-custom-board -t upload
   ```

## Dependencies

All variants include:
- **BresserWeatherSensorReceiver** — Sensor decoder library
- **RadioLib** — 868 MHz radio driver (auto-included by BresserWeatherSensorReceiver)
- **WiFiManager** — Web-based WiFi + configuration portal
- **Adafruit BMP280 Library** — Optional pressure sensor support

## Troubleshooting

### "Board not found" error
- Ensure the board is listed in Arduino Board Manager
- Check that `platform = espressif32` is correct
- Update PlatformIO: `pio system update`

### Upload fails
- Try selecting the correct COM port in PlatformIO
- Press and hold the **BOOT** button while uploading (some boards)
- Try a different USB cable

### Sensor not detected
- Verify antenna is connected and not damaged
- Check that the correct radio module (SX1276/SX1262) is selected
- Ensure Bresser sensor batteries are fresh
- Monitor serial output for initialization messages

### Wrong pins configured
- Verify pins match your board's pinout
- Update `PIN_RECEIVER_*` definitions in `WeatherSensorCfg.h`
- Check RadioLib documentation for your radio chip

## Resources

- [PlatformIO Docs](https://docs.platformio.org)
- [Arduino ESP32 Cores](https://github.com/espressif/arduino-esp32)
- [RadioLib Documentation](https://jgromes.github.io/RadioLib/)
- [BresserWeatherSensorReceiver](https://github.com/matthias-bs/BresserWeatherSensorReceiver)

---

**Happy building!** 🚀
