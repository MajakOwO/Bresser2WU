# Bresser Weather Sensor to WU + APRS-IS Gateway

[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE) [![CI](https://github.com/MajakOwO/Bresser2WU/actions/workflows/CI.yml/badge.svg)](https://github.com/MajakOwO/Bresser2WU/actions/workflows/CI.yml)

A weather station gateway that receives data from Bresser wireless weather sensors and simultaneously forwards observations to:
- **Weather Underground (WU)** - for weather dashboard integration
- **APRS-IS Network** - for amateur radio integration and real-time mapping via [aprs.fi](https://aprs.fi)

Built on ESP32 with PlatformIO, featuring WiFiManager for easy configuration.

## Features

- 📡 **Dual Service Reporting**
  - Weather Underground: Every 15 seconds
  - APRS-IS: Every 10 minutes
  
- ⚙️ **Easy Configuration**
  - Web-based WiFiManager for WiFi and API credentials
  - No code changes needed - all settings via captive portal
  - Credentials stored in ESP32 NVS (non-volatile storage)

- 🌡️ **Rich Weather Data**
  - Temperature, humidity, wind speed/gust/direction
  - Rainfall (hourly + daily tracking)
  - Barometric pressure (BMP280 sensor)
  - Solar radiation & UV index
  - Device battery status & signal strength (RSSI)

- 📊 **Rain Gauge History**
  - 60-minute buffer for hourly rainfall calculations
  - Daily total with midnight rollover detection

- 🔋 **Health Metrics**
  - Battery status (OK/Low) displayed in APRS beacon
  - RSSI signal strength in APRS comment

- 📱 **Multi-Board Support**
  - Supports 10+ ESP32 board variants (LILYGO TTGO, Heltec, Adafruit, M5Stack, LilyGo T3-S3, and more)
  - Easy variant selection in `platformio.ini`
  - See [Board Variants Guide](docs/BOARD_VARIANTS.md) for details

## Hardware Requirements

- **ESP32** microcontroller (WROOM or equivalent)
- **Bresser 7-in-1 Weather Sensor** (868 MHz receiver included)
- **Optional: BMP280** pressure sensor (I2C: GPIO 16=SDA, GPIO 17=SCL)
- 868 MHz antenna for sensor reception
- USB power or battery supply

## Software Setup

### Prerequisites
- [PlatformIO](https://platformio.org) (VS Code extension or CLI)
- Python 3.6+ (for PlatformIO)

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/MajakOwO/Bresser2WU.git
   cd Bresser2WU
   ```

2. **Select your board variant** (optional)
   - The project supports many ESP32 boards (LILYGO TTGO, Heltec, Adafruit, M5Stack, LilyGo T3-S3, and others)
   - See [**Board Variants Guide**](docs/BOARD_VARIANTS.md) for the current list and mapping to `WeatherSensorCfg.h`
   - Default environment is `esp32dev` with SX1276 radio pinning
   - To build for a different hardware variant, choose the matching `[env:*]` in `platformio.ini`

3. **Build and upload**
   - Build the default environment:
     ```bash
     pio run
     ```
   - Build and upload a specific variant:
     ```bash
     pio run -e heltec-wireless-stick -t upload
     ```
   Or use VS Code PlatformIO tasks

4. **Configure via WiFiManager**
   - Power on the ESP32
   - Connect to `BresserWX` WiFi AP
   - Open browser to `192.168.4.1`
   - Enter WiFi SSID and password
   - Configure Weather Underground: Station ID + Key
   - Configure APRS: Callsign (e.g., SQ8NA-15), Password, Latitude, Longitude, Comment (optional)
   - Save and restart

## Configuration Details

### Weather Underground
- **Station ID**: Your WU station identifier (e.g., `KZZXY50`)
- **Key**: API key from Weather Underground account
- Data sent every 15 seconds to `wunderground.com`

### APRS-IS
- **Callsign**: APRS identifier with optional SSID (e.g., `SQ8NA-15`)
- **Password**: Numeric code for APRS-IS authentication
- **Latitude/Longitude**: Station position in decimal format (e.g., `XX.XXXX` / `XX.XXXX`)
- **Comment**: Optional comment (up to 64 chars, appears in beacon)
- Server: `rotate.aprs.net:14580` with automatic fallback on connection issues
- Data sent every 10 minutes

### APRS Packet Format
```
!DDMM.hhN/DDDMM.hhW_ddd/sssgggtXXXhXXrXXXpXXX Bat:OK RSSI:-110.5dBm
```
- Position: Latitude/Longitude in degrees/minutes
- Wind: Direction / Speed / Gust (mph)
- Temperature: Fahrenheit
- Humidity, Rain, Pressure encoded per [APRS spec](https://www.aprs.org/aprs11/spec-wx.txt)
- Health: Battery status (OK/Low) and RSSI in comment

## Project Structure

```
src/
  ├── main.cpp              # Main application logic
  ├── APRS.h / APRS.cpp     # APRS-IS TCP client
  ├── config.h              # Configuration constants
  ├── InitBoard.h/cpp       # Board initialization
  └── WeatherSensor*        # BresserWeatherSensorReceiver wrapper
  
data/
  ├── index.html            # Web interface (optional)
  ├── style.css
  └── script.js

examples/
  └── BresserWeatherSensor* # Reference implementations
```

## Usage

### Monitoring Live
- **Weather Underground**: Visit your WU dashboard
- **APRS**: Search callsign on [aprs.fi](https://aprs.fi)

## Building from Source

### Development Workflow
```bash
# Build
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor --baud 115200

# Clean build
pio run -t clean
```

### Custom Board Configuration
Edit `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
; ... (additional settings)
```

## Dependencies

- **BresserWeatherSensorReceiver** - Weather sensor decoder
- **RadioLib** (7.6.0) - 868 MHz radio interface
- **WiFiManager** - WiFi configuration
- **Adafruit BMP280** - Pressure sensor driver (optional)

## Troubleshooting

### APRS Connection Issues
- Verify callsign and password are correct
- Check latitude/longitude format (decimal, not DMS)
- Ensure ESP32 has stable WiFi connection
- Check Serial monitor for DNS resolution and auth errors

### Weather Underground Issues
- Verify Station ID and Key in WU dashboard
- Check that WiFi is connected before transmitting
- Confirm `https://` endpoint is accessible from your network

### Rain Gauge Anomalies
- Check if sensor reports valid rainfall data
- Watch for midnight rollover if running 24/7
- Monitor Serial debug output for rainHistory buffer status

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test thoroughly
4. Submit a pull request with description

## Credits

- Built on [BresserWeatherSensorReceiver](https://github.com/matthias-bs/BresserWeatherSensorReceiver) by Matthias Prinke
- APRS specification: https://www.aprs.org
- RadioLib: https://github.com/jgromes/RadioLib

## Support

For issues, questions, or feature requests:
- Check [closed issues](https://github.com/MajakOwO/Bresser2WU/issues)
- Review [APRS packet format reference](https://www.aprs.org/aprs11/spec-wx.txt)
- Consult Weather Underground API documentation

---

**Happy weather monitoring!** 🌤️📡