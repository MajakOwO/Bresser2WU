# Changelog

All notable changes to the Bresser Weather Sensor to WU + APRS-IS Gateway project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026

### Added

- **Initial Release**: Complete Weather Station Gateway
  - Bresser 7-in-1 weather sensor receiver (868 MHz)
  - Dual service reporting:
    - **Weather Underground**: HTTP POST every 15 seconds
    - **APRS-IS**: TCP beacon every 10 minutes
  
- **WiFiManager Configuration**
  - Web-based captive portal setup (AP: `BresserWX`)
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
  - APRS weather packet format: `!DDMM.hhN/DDDMM.hhW_ddd/sssgggtXXXhXXrXXXpXXX`
  - Device health metrics: Battery status (OK/Low) and RSSI in beacon comment
  - Per-spec encoding: humidity (00=100%, 01-99=percent), wind (mph), temperature (°F), rain (0.01"), pressure (inHg)

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
