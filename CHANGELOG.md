# Changelog

All notable changes to the Bresser Weather Sensor to WU + APRS-IS Gateway project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-06-16

### Added

- **OTA Firmware Updates**
  - Built-in OTA update server available on port `8080`
  - OTA update link exposed on the web configuration portal
  - Supports firmware upload without USB connection

- **Built-in Web Configuration Portal**
  - Custom configuration page for WiFi, Weather Underground, APRS, and MQTT settings
  - Status dashboard shows WiFi, APRS, sensor, and OTA state
  - Stored settings persist in ESP32 NVS non-volatile storage

- **MQTT Support**
  - New `sendToMQTT()` function publishes weather data to MQTT broker
  - Configurable MQTT broker address, port, topic, username, password
  - JSON payload format with `ArduinoJson` library integration
  - Settings stored in ESP32 NVS (non-volatile storage)
  - Automatic reconnection with buffer size optimization (512 bytes)

- **Extended OLED Display (SSD1306)**
  - Boot screen with repository link: `MajakOwO/Bresser2WU`
  - Rich weather data display (7 lines):
    - Temperature (°C) and humidity (%)
    - Dew point (°C) and barometric pressure (hPa)
    - Wind speed/gust (m/s) and direction (degrees)
    - Daily and hourly rainfall (mm)
    - Precipitation rate (mm/h) and solar radiation (W/m²)
    - WiFi and hardware status
    - IP address or disconnected status
  - Hardware status indicators:
    - `LoRa ERROR` when radio initialization fails
    - `BMP280 OFF` when pressure sensor is not detected
    - `NO WIFI` when WiFi credentials are not configured
    - `WIFI FAILED` when WiFi connection attempt failed
    - `Status: OK` when all systems operational

- **Test Weather Mode (TEST_WEATHER)**
  - New `esp32dev-test-weather` environment with emulated weather data
  - `generateTestWeatherData()` function provides synthetic values for testing
  - Allows firmware testing without active Bresser sensor
  - Compatible with WU, APRS, and MQTT transmission
  - Build flag: `-D TEST_WEATHER`

- **WiFi Status Tracking**
  - Global flags: `wifiConfigured`, `wifiConnected`, `loraPresent`, `loraInitFailed`
  - Real-time status updates on OLED display
  - Helps diagnose connection and hardware issues

### Changed

- **platformio.ini Structure**
  - Fixed invalid top-level `lib_deps` section (moved to per-environment)
  - Added `adafruit/Adafruit SSD1306` library to all environments
  - Added `knolleary/PubSubClient` to all environments (for MQTT)

- **OLED Layout Optimization**
  - Consolidated weather data to fit 128x64 display
  - Grouped related measurements on single lines (T+H, DP+P, W+D)
  - Separate lines for error messages
  - Improved font sizing and positioning

- **Configuration Portal**
  - Added status dashboard and OTA update link to the web config page
  - Improved on-device feedback for WiFi, APRS, and sensor health

### Fixed

- Precipitation rate calculation now uses 60-second baseline intervals to reduce erratic updates
- MQTT buffer size insufficient for JSON weather payloads (increased to 512 bytes)
- `platformio.ini` per-environment dependencies fixed for all board variants

## [1.0.2] - 2026-06-11

### Fixed

- Weather Underground: precipitation "Precip Rate" is now sent as `precipratein` (mm/h -> in/h) calculated from delta of cumulative rain over time.
  - Previously `rainin` was effectively treated as rate.

## [1.0.1] - 2026-06-08

### Changed

- APRS wind speed and gust transmission changed to meters per second (m/s)
- APRS weather packet now includes barometric pressure as `Pxxxxx` (tenths of millibars)


## [1.0.0] - 2026

### Added

- **Initial Release**: Complete Weather Station Gateway
  - Bresser 7-in-1 weather sensor receiver (868 MHz)
  - Dual service reporting:
    - **Weather Underground**: HTTP POST every 15 seconds
    - **APRS-IS**: TCP beacon every 10 minutes
  
- **Web Configuration Portal**
  - Custom web page available when AP `BresserWX` is active
  - Configurable Weather Underground credentials (Station ID, API Key)
  - Configurable APRS-IS parameters (Callsign, Password, Latitude, Longitude, Comment)
  - Settings persisted in ESP32 NVS storage

- **Weather Data Processing**
  - Temperature, humidity, wind measurements (speed, gust, direction)
  - Rainfall tracking: hourly (60-minute buffer) + daily with midnight rollover
  - Barometric pressure (optional BMP280 sensor support)
  - Solar radiation and UV index
  - Dew point calculation (Magnus formula)
  - Unit conversions: Metric ↔ Imperial

- **APRS Features**
  - TCP connection to `rotate.aprs.net:14580`
  - Automatic reconnection with 5-minute retry interval
  - APRS weather packet format: `!DDMM.hhN/DDDMM.hhW_ddd/sssgggtXXXhXXPXXXXXrXXXpXXX`

  - Device health metrics: Battery status (OK/Low) and RSSI in beacon comment
  - Per-spec encoding: humidity (00=100%, 01-99=percent), wind (m/s), temperature (°F), rain (0.01"), pressure (Pxxxxx in tenths of millibars)

- **Sensor Support**
  - Bresser 7-in-1 weather sensor (example: sensor ID A86A)
  - Multiple sensor capability (configurable via `MAX_SENSORS_DEFAULT`)
  - Sensor status reporting: battery, signal strength (RSSI)

- **Serial Monitoring**
  - Comprehensive debug output at 115200 baud
  - Sensor data display with all parameters
  - Connection status and error reporting
  - Rain gauge buffer status tracking

- **Documentation**
  - Comprehensive README with features, setup, and troubleshooting
  - BUILD.md: PlatformIO build instructions
  - CONTRIBUTING.md: Developer guidelines
  - CHANGELOG.md: Version history (this file)

### Technical Details

- **Platform**: ESP32 with Arduino framework
- **Build System**: PlatformIO
- **CI/CD**: Added GitHub Actions workflow to precompile firmware artifacts for supported board variants
- **Core Libraries**:
  - BresserWeatherSensorReceiver (sensor decoding)
  - RadioLib 7.6.0 (868 MHz radio interface)
  - WiFiManager (WiFi configuration)
  - HTTPClient (Weather Underground API)
  - Preferences (NVS storage)
  - Adafruit_BMP280 (optional pressure sensor)

- **Memory Usage**: ~180 KB RAM, ~850 KB flash (ESP32)
- **Code Size**: ~1.3 MB compiled binary

### Configuration

- **WU Update Interval**: 15 seconds
- **APRS Update Interval**: 10 minutes (600 seconds)
- **APRS Retry Interval**: 5 minutes (300 seconds)
- **Rain Gauge Buffer**: 60 samples (hourly resolution)
- **WiFiManager AP Timeout**: 300 seconds
- **Time Zone**: UTC+1 with DST support
- **NTP Server**: pool.ntp.org

## Known Limitations

- Single sensor support (first sensor in array)
- Rain gauge hourly/daily calculation requires accurate time (NTP)
- BMP280 pressure is optional and may be unavailable
- APRS 24-hour rain value uses hourly rain as placeholder (one-hour reporting cycle)

---

## How to Report Issues

Please report bugs and suggest features via [GitHub Issues](https://github.com/MajakOwO/Bresser2WU/issues).

Include:
- Hardware configuration
- Serial monitor output
- Steps to reproduce
- Expected vs actual behavior

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Code style
- Testing requirements
- Pull request process
- Development workflow

---

**Changelog Syntax**: Added | Changed | Deprecated | Removed | Fixed | Security
