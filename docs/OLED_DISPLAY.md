# OLED Display Configuration Guide

## Overview

The firmware supports an optional **SSD1306 OLED display** (128x64 pixels) connected via I2C, providing real-time visualization of weather data, system status, and network connectivity.

## Hardware Setup

### Display Module
- **Type**: SSD1306 OLED (128x64 pixels)
- **Interface**: I2C
- **Default I2C Pins**:
  - **SDA**: GPIO 16
  - **SCL**: GPIO 17
- **Typical I2C Address**: `0x3C` (configurable in code)

### Wiring

```
SSD1306    ESP32
---------  ----------
VCC        3.3V
GND        GND
SDA        GPIO 16
SCL        GPIO 17
```

## Firmware Requirements

Add the following library to your `platformio.ini` per-environment `lib_deps`:

```ini
[env:esp32dev]
lib_deps =
    adafruit/Adafruit SSD1306
```

**Note**: The global `lib_deps` section does NOT propagate dependencies. Each environment must have its own `lib_deps` declaration.

## Display Layout

The OLED shows a 7-line weather display updated in real-time:

```
Line 1: T:20.3C H:45%
Line 2: DP:7.9C P:1013.2hPa
Line 3: W:1.7/1.8 m/s D:138
Line 4: Rain:2.5mm 24h:2.5
Line 5: Rate:0.5mm/h SR:450
Line 6: Status: OK
Line 7: IP:192.168.1.100
```

### Data Fields

| Field | Description | Units |
|-------|-------------|-------|
| **T** | Temperature | °C |
| **H** | Humidity | % |
| **DP** | Dew Point | °C |
| **P** | Barometric Pressure | hPa |
| **W** | Wind Speed / Gust | m/s |
| **D** | Wind Direction | degrees (0-360) |
| **Rain** | Daily Rainfall | mm |
| **24h** | Hourly Rainfall | mm |
| **Rate** | Precipitation Rate | mm/h |
| **SR** | Solar Radiation | W/m² |

### Status Messages

The display shows different status messages on **Line 6** based on system state:

| Message | Condition | Action |
|---------|-----------|--------|
| `Status: OK` | All systems operational | None needed |
| `NO WIFI` | WiFi credentials not configured | Configure via captive portal |
| `WIFI FAILED` | WiFi connection attempt failed | Check SSID/password, signal strength |
| `LoRa ERROR` | Radio module initialization failed | Check wiring, module type |
| `BMP280 OFF` | Pressure sensor not detected | Check I2C wiring (different pins?) |

### Network Status (Line 7)

- **Connected**: Shows `IP:X.X.X.X`
- **Disconnected**: Shows `WiFi: disconnected`

## Boot Screen

When the device starts, the OLED displays a welcome screen:

```
booting...

Bresser2WU


MajakOwO/Bresser2WU
```

This screen appears for ~2-3 seconds before transitioning to the live weather display.

## Initialization

The display is automatically initialized at startup via the `setupDisplay()` function:

1. Initializes SSD1306 with address `0x3C`
2. Clears the display
3. Shows the boot screen
4. Transitions to weather data after boot sequence completes

## Disabling the Display

If you don't have an OLED display connected:

1. **Option A**: Leave the display unconnected—firmware will attempt initialization but continue operation if it fails
2. **Option B**: Remove `Adafruit SSD1306` from `lib_deps` and comment out `#define USE_SSD1306` in code (if present)

The firmware will continue operating normally without the display.

## Troubleshooting

### Display Not Showing

1. **Check I2C address**: Run I2C scanner to confirm display address (usually `0x3C`)
2. **Check wiring**: Verify SDA/SCL are connected to GPIO 16/17
3. **Check power**: Ensure 3.3V is supplied
4. **Check library**: Verify `adafruit/Adafruit SSD1306` is in `lib_deps`

### Display Shows Garbled Text

- **Cause**: I2C initialization issue or address mismatch
- **Solution**: Check I2C wiring, verify address with I2C scanner

### Display Shows Only Some Data

- **Cause**: Font rendering or buffer issue
- **Solution**: Recompile firmware and upload again

### "LoRa ERROR" Appears but Radio Works

- **Cause**: Initialization timing issue
- **Solution**: This is informational; if sensor data is being received, the radio is working

## Performance Impact

- **Rendering time**: ~10 ms per update cycle
- **Memory usage**: ~8 KB for display buffer
- **CPU usage**: Minimal (idle wait for I2C)
- **Update frequency**: Every loop cycle (~100 ms typical)

## Customization

To modify the display layout, edit the `renderDisplay()` function in `src/main.cpp`:

```cpp
void renderDisplay() {
  // Modify text content, positions, sizes here
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Custom Data Here");
  display.display();
}
```

**Font sizes**:
- `setTextSize(1)`: Small (5x7 pixels per char)
- `setTextSize(2)`: Large (10x14 pixels per char)

**Common display methods**:
- `display.setCursor(x, y)` - Position for next text
- `display.print(text)` - Write text
- `display.display()` - Push buffer to OLED
- `display.clearDisplay()` - Clear all pixels
- `display.fillRect(x, y, w, h, color)` - Draw rectangle
- `display.drawPixel(x, y, color)` - Draw single pixel

## Technical Details

- **Resolution**: 128x64 pixels
- **Display driver**: SSD1306 (I2C)
- **Typical refresh rate**: 100 ms (10 Hz)
- **Supported fonts**: 5x7 (size 1) and 10x14 (size 2) monospace
- **Colors**: Monochrome (black/white, 1-bit)
