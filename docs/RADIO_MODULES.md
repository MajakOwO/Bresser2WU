# Radio Modules Guide

This project supports multiple LoRa radio modules for 868 MHz weather sensor reception. Choose the module that fits your hardware setup.

## Supported Radio Modules

### SX1262 (Recommended for new builds)
- **Manufacturer**: Semtech
- **Features**:
  - Newer, more power-efficient than SX1276
  - Better performance in low-power scenarios
  - Integrated BUSY signal for reliable operation
  - Superior range and sensitivity
- **Used on**: Heltec Wireless Stick V3, Heltec WiFi LoRa 32 V3/V4, LilyGo T3-S3 (SX1262 variant)
- **Typical Pinout** (SPI):
  ```
  MOSI  → GPIO 23
  MISO  → GPIO 19
  SCK   → GPIO 18
  CS    → GPIO 27 (adjustable)
  IRQ   → GPIO 21 (adjustable)
  BUSY  → GPIO 33 (adjustable)
  RST   → GPIO 32 (adjustable)
  ```

### SX1276 (RFM95W)
- **Manufacturer**: Semtech
- **Features**:
  - Widely available
  - Proven and stable
  - Good range for amateur use
  - Compatible with many existing projects
- **Used on**: LILYGO TTGO LoRa32 V1/V2/V2.1, Heltec Wireless Stick, Heltec WiFi LoRa 32 V2, Adafruit Feather ESP32
- **Typical Pinout** (SPI):
  ```
  MOSI  → GPIO 23
  MISO  → GPIO 19
  SCK   → GPIO 18
  CS    → GPIO 27 (adjustable)
  DIO0  → GPIO 21 (adjustable)
  DIO1  → GPIO 33 (adjustable)
  RST   → GPIO 32 (adjustable)
  ```

### LR1121 (Latest LoRa Technology)
- **Manufacturer**: Semtech
- **Features**:
  - Newest LoRa chip
  - Multi-band support
  - Superior performance
  - Lower power consumption
- **Used on**: LilyGo T3-S3 (LR1121 variant)
- **Typical Pinout**:
  ```
  MOSI  → GPIO 23
  MISO  → GPIO 19
  SCK   → GPIO 5
  CS    → GPIO 10
  IRQ   → GPIO 21 (DIO0)
  BUSY  → GPIO 8
  RST   → GPIO 9
  ```

### CC1101 (Alternative Option)
- **Manufacturer**: Texas Instruments
- **Features**:
  - Alternative to LoRa technology
  - Lower power consumption than LoRa
  - Good performance on amateur frequencies
  - Less common but supported
- **Used on**: Custom builds with CC1101 modules
- **Note**: Configuration depends on specific module wiring

## Selecting a Radio Module for ESP32 Dev Board

The generic **esp32dev** environment supports all four radio modules. Select which one via `platformio.ini`:

```ini
[env:esp32dev-sx1262]
build_flags = -D ESP32_VARIANT_SX1262

[env:esp32dev-sx1276]
build_flags = -D ESP32_VARIANT_SX1276

[env:esp32dev-cc1101]
build_flags = -D ESP32_VARIANT_CC1101

[env:esp32dev-lr1121]
build_flags = -D ESP32_VARIANT_LR1121
```

## GPIO Pin Configuration

### Default ESP32 DevKit Pinout (Compatible with all modules)

| Purpose | GPIO | Notes |
|---------|------|-------|
| SPI MOSI | 23 | Data to module |
| SPI MISO | 19 | Data from module |
| SPI SCK | 18 | Clock signal |
| **Module Chip Select** | 27 | Active low |
| **Module Interrupt/DIO0** | 21 | Activity signal |
| **Module GPIO/DIO1/BUSY** | 33 | Activity/Busy signal |
| **Module Reset** | 32 | Active low |

### Custom GPIO Configuration

To use different GPIO pins, edit [src/WeatherSensorCfg.h](../src/WeatherSensorCfg.h):

```cpp
#elif defined(ESP32)
    #define ESP32_VARIANT_SX1262
    
    #if defined(ESP32_VARIANT_SX1262)
        #define USE_SX1262
        #define PIN_RECEIVER_CS   27   // Adjust here
        #define PIN_RECEIVER_IRQ  21   // Adjust here
        #define PIN_RECEIVER_GPIO 33   // Adjust here
        #define PIN_RECEIVER_RST  32   // Adjust here
```

## Module Selection Comparison

| Feature | SX1262 | SX1276 | LR1121 | CC1101 |
|---------|--------|--------|--------|---------|
| **Power Efficiency** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Range** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Availability** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| **Cost** | $ | $ | $$ | $ |
| **Ease of Use** | Easy | Easy | Easy | Medium |
| **Recommended For** | New builds | Budget/compatibility | Advanced users | Alternative option |

## Troubleshooting Radio Module Issues

### No Signal Received
1. **Check wiring** - Verify all GPIO connections match your configuration
2. **Antenna connection** - Ensure antenna is properly connected and not damaged
3. **Frequency calibration** - Verify 868 MHz frequency is correctly set
4. **Radio module power** - Ensure 3.3V supply has sufficient current

### Intermittent Reception
1. **SPI bus conflicts** - Check if other devices use same SPI pins
2. **Power supply stability** - Use separate power for the radio module if possible
3. **Antenna quality** - Consider upgrading to high-gain antenna
4. **Cable length** - Keep SPI connections short (< 10cm recommended)

### Build Errors
1. **RadioLib library** - Ensure RadioLib is installed: `pio lib install "jgromes/RadioLib"`
2. **Compilation flags** - Verify correct `-D ESP32_VARIANT_*` flag is set
3. **Board definition** - Check WeatherSensorCfg.h has matching board configuration

## Additional Resources

- **RadioLib Documentation**: https://jgromes.github.io/RadioLib/
- **Semtech SX1262 Datasheet**: https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262
- **Semtech SX1276 Datasheet**: https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276
- **Community Support**: Check GitHub Issues for module-specific questions
