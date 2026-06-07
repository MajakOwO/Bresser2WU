# Build Instructions - Bresser Weather Sensor to WU + APRS-IS Gateway

This project uses **PlatformIO** for building and uploading to ESP32.

## Prerequisites

- **Visual Studio Code** with **PlatformIO IDE** extension
- **Python 3.6+** (required by PlatformIO)
- **ESP32 Development Board** (e.g., ESP32-WROOM-32)
- **USB cable** for flashing and serial communication
- Bresser 7-in-1 Weather Sensor (868 MHz receiver)
- Optional: BMP280 pressure sensor

## Quick Start

### 1. Install PlatformIO

- Open VS Code
- Go to Extensions (Ctrl+Shift+X)
- Search for "PlatformIO IDE" by PlatformIO
- Click Install

### 2. Clone and Open Project

```bash
git clone https://github.com/MajakOwO/Bresser2WU.git
cd Bresser2WU
code .
```

### 3. Build the Project

- Click the **PlatformIO** icon in the left sidebar
- Click **Build** or press `Ctrl+Alt+B`

### 4. Upload to ESP32

**First time only:** Select your environment in `platformio.ini`

```bash
# Or use terminal
pio run -t upload
```

Or use the PlatformIO sidebar → **Upload**

### 5. Monitor Serial Output

- PlatformIO sidebar → **Serial Monitor** 
- Or: `pio device monitor --baud 115200`

## Configuration

### Board Selection

Edit `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev           # Change to your board type
framework = arduino
monitor_speed = 115200
```

Common ESP32 boards:
- `esp32dev` - Generic ESP32 DevKit
- `esp32wrover-kit` - ESP32 WROVER Kit
- `esp-wroom-32` - Standalone WROOM-32

### WiFiManager Setup

After first upload:

1. Look for `BresserWX` WiFi access point
2. Connect to it
3. Open browser to `192.168.4.1`
4. Configure:
   - WiFi SSID & Password
   - Weather Underground: Station ID and API Key
   - APRS-IS: Callsign, Password, Latitude, Longitude, Comment
5. Save and restart

All settings persist in ESP32 NVS storage.

### Optional: BMP280 Pressure Sensor

If you have a BMP280 sensor connected via I2C (GPIO 16=SDA, GPIO 17=SCL):

- The code automatically detects it at startup
- Pressure data is included in both WU and APRS transmissions
- Serial output shows: `BMP280 OK` or `BMP280 NOT FOUND`

No code changes needed - it's enabled by default.

### Disable BMP280 (Optional)

In `src/main.cpp`, comment out:

```cpp
//#define USE_BMP280
```

Then rebuild and upload.

## Development Workflow

### Building

```bash
# Full build
pio run

# Clean build
pio run -t clean

# Specific environment
pio run -e esp32dev
```

### Uploading

```bash
# Upload to default environment
pio run -t upload

# Upload and monitor
pio run -t upload && pio device monitor --baud 115200

# Or in two steps
pio run -t upload
pio device monitor --baud 115200
```

### Debugging

Monitor serial output for:

- **Startup**: Board initialization, sensor detection, WiFi mode
- **WiFiManager**: Configuration portal timeout, WiFi connection
- **Sensor Data**: Weather sensor reception, data calculations
- **APRS**: Connection attempts, authentication, packet transmission
- **WU**: HTTP response codes, data submission
- **Errors**: Connection failures, sensor errors, data validation

## Troubleshooting Build Issues

### Serial Port Not Found

**Windows:**
- Install [CH340 driver](https://sparks.gogo.co.nz/ch340.html) if using cheap devboards
- Check Device Manager for COM ports
- In `platformio.ini`: `upload_port = COM3` (adjust number)

**Linux/Mac:**
- `ls /dev/tty.* ` or `ls /dev/ttyUSB*` to find port
- Ensure user is in `dialout` group: `sudo usermod -aG dialout $USER`

### Upload Fails

- Check USB cable (try different one)
- Hold `BOOT` button while uploading (some boards)
- Try different USB port on computer
- Increase timeout in `platformio.ini`: `upload_speed = 115200`

### Out of Memory / Compilation Error

- Ensure using the latest RadioLib (7.6.0)
- Check that unnecessary includes are removed
- In extreme cases, remove optional features (BMP280, etc.)

### Sensor Not Detected

- Check 868 MHz antenna connection
- Verify antenna is not damaged
- Ensure Bresser sensor batteries are fresh
- Check sensor ID in serial output

## Performance Notes

- **Build Time**: ~30-60 seconds on first build, ~5-10 seconds for incremental
- **Upload Time**: ~10-15 seconds via USB
- **Memory**: ~850 KB flash, ~180 KB RAM (out of 520 KB available)
- **Build Size**: ~1.3 MB compiled binary

## Useful PlatformIO Commands

```bash
# List connected devices
pio device list

# Check board info
pio boards esp32dev

# Open platformio.ini documentation
pio settings describe

# Global PlatformIO installation info
pio system info
```

## IDE Tasks in VS Code

PlatformIO adds these to VS Code tasks (Ctrl+Shift+P → Tasks):

- **PlatformIO: Build**
- **PlatformIO: Upload**
- **PlatformIO: Upload and Monitor**
- **PlatformIO: Clean**
- **PlatformIO: Set Default Project**

## Further Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [BresserWeatherSensorReceiver](https://github.com/matthias-bs/BresserWeatherSensorReceiver)
- [Weather Underground API](https://www.wunderground.com/weather/api/)
- [APRS.org](https://www.aprs.org)

---

**Built with ❤️ using PlatformIO**