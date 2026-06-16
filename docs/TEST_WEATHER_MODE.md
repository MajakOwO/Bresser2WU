# Test Weather Mode (TEST_WEATHER) Guide

## Overview

The **TEST_WEATHER** build variant allows you to test and develop firmware without an active Bresser weather sensor. It uses emulated weather data that cycles through realistic values, enabling testing of:

- Weather Underground (WU) integration
- APRS-IS transmission
- MQTT publishing
- OLED display rendering
- WiFi and network connectivity
- Data processing and calculations

## Building TEST_WEATHER Firmware

### Option 1: PlatformIO CLI

```bash
cd Bresser2WU
pio run -e esp32dev-test-weather -t upload --port COM3
```

### Option 2: VS Code PlatformIO Extension

1. Open the PlatformIO sidebar
2. Select **Projects** → **Bresser2WU**
3. Click the environment dropdown
4. Choose `esp32dev-test-weather`
5. Click **Build** or **Upload**

### Option 3: Manual Build Configuration

Edit `platformio.ini` to use TEST_WEATHER flag:

```ini
[env:esp32dev]
build_flags = -D ESP32_VARIANT_SX1262 -D TEST_WEATHER
```

## Emulated Data Values

The `generateTestWeatherData()` function produces the following hardcoded values:

```cpp
float temperature_c = 20.3;      // 20.3°C (68.5°F)
float humidity = 45;              // 45% relative humidity
float wind_speed_ms = 1.7;        // 1.7 m/s (SW direction)
float wind_gust_ms = 1.8;         // 1.8 m/s gust
float wind_direction = 138;       // 138° (SE)
float pressure_hpa = 1013.25;     // Normal atmospheric
float rain_daily_mm = 2.5;        // 2.5mm daily total
float rain_hourly_mm = 0.5;       // 0.5mm in last hour
float precip_rate_mmh = 0.25;     // 0.25 mm/h rate
float solar_radiation_wpm2 = 450; // 450 W/m²
```

These values are updated in `sendToWU()` and `sendToAPRS()` calls during each loop iteration.

## Behavior Changes

When compiled with `-D TEST_WEATHER`, the firmware behavior changes as follows:

### 1. Sensor Reception Skipped

```cpp
#ifdef TEST_WEATHER
  // Skip actual radio reception
  generateTestWeatherData();
  haveSensorData = true;
#else
  // Normal operation: receive from Bresser sensor
  if (ws.getMessage()) {
    haveSensorData = true;
    // ... process real sensor data
  }
#endif
```

**Implication**: The LoRa radio is still initialized but not actively used.

### 2. WU Transmission Uses Fixed Rain Data

```cpp
#ifdef TEST_WEATHER
  // Use emulated rain_daily_mm value
#else
  // Normal: read from rolling counter and sensor
  rain_daily_mm = ...
#endif
```

**Implication**: Rain data is not calculated from sensor; fixed values are used.

### 3. APRS Transmission Bypasses Sensor Validity Check

```cpp
#ifdef TEST_WEATHER
  // Skip sensor.temp_ok validation, use fixed battery_ok=true
  data.battery_ok = true;
  data.rssi = -50;  // Simulated signal strength
#else
  // Normal: check sensor validity before sending
  if (sensor.temp_ok) { ... }
#endif
```

**Implication**: APRS beacons are always sent regardless of sensor state.

### 4. Display Always Shows "Status: OK"

Since sensor checks are bypassed, the OLED display shows:
- No `LoRa ERROR` message
- No `BMP280 OFF` message
- Always displays `Status: OK` (if WiFi is configured)

## Usage Scenarios

### Scenario 1: Test WiFi Configuration

1. Build and upload `esp32dev-test-weather`
2. Connect to `BresserWX` WiFi AP
3. Configure WiFi and WU/APRS credentials
4. Verify data appears in Weather Underground and APRS.fi without a sensor

### Scenario 2: Develop MQTT Integration

1. Build with `-D TEST_WEATHER -D USE_MQTT` (or equivalent)
2. Configure MQTT broker address
3. Monitor MQTT topic for weather data:
   ```bash
   mosquitto_sub -h 192.168.1.100 -t weather/station -v
   ```
4. Adjust JSON format or fields without needing sensor data

### Scenario 3: Test OLED Display

1. Build `esp32dev-test-weather` with `-D USE_SSD1306`
2. Connect OLED display to I2C pins
3. Verify all weather fields render correctly
4. Adjust font sizes and formatting

### Scenario 4: Debug Network Issues

1. Build `esp32dev-test-weather`
2. Monitor serial output (115200 baud):
   ```bash
   picocom /dev/ttyUSB0 -b 115200
   ```
3. Watch WiFi connection attempts and retries without sensor interference

### Scenario 5: Performance Testing

1. Build `esp32dev-test-weather`
2. Measure power consumption, WiFi signal, transmission success rates
3. Profile CPU and memory usage without sensor variability

## Serial Output Examples

When running `esp32dev-test-weather`, expect output like:

```
[SETUP] Initializing sensors...
[SETUP] Sensor initialization completed
[MAIN] Sensor not found, but TEST_WEATHER enabled
[MAIN] haveSensorData = true (TEST_WEATHER mode)
[WU] Sending to Weather Underground: T=20.3C, H=45%, P=1013.25hPa
[APRS] Beacon sent: !DDMM.hhN/DDDMM.hhW_ddd/sss...
[MQTT] Publish OK: {"temperature_c": 20.3, ...}
[DISPLAY] Rendering weather data...
```

## Switching Back to Normal Mode

To go back to normal sensor mode:

1. **Edit platformio.ini**: Remove `-D TEST_WEATHER` flag
2. **Or select a different environment**: e.g., `esp32dev-sx1262`
3. Rebuild and upload

## Limitations

- **No real sensor data**: All weather values are static
- **No rain variability**: Rainfall always shows fixed 2.5mm daily
- **Simulated signal strength**: RSSI always -50 dBm
- **No multi-sensor testing**: Only one hardcoded data set

## Advanced Customization

To change the emulated data values, edit `generateTestWeatherData()` in `src/main.cpp`:

```cpp
void generateTestWeatherData() {
  // Modify these values to test different conditions
  humidity = 65.0;  // Higher humidity
  temperature_c = 25.0;  // Warmer temperature
  rain_daily_mm = 10.0;  // Heavy rain scenario
  wind_speed_ms = 8.5;  // Strong wind
}
```

Examples for testing different conditions:

```cpp
// Cold weather test
temperature_c = -5.0;
humidity = 85.0;
wind_gust_ms = 12.0;

// Rain scenario
rain_daily_mm = 15.0;
precip_rate_mmh = 2.0;

// Sunny conditions
solar_radiation_wpm2 = 950.0;
humidity = 30.0;
```

## Troubleshooting

### No Data Appearing in WU Dashboard

1. **Check credentials**: Verify Station ID and API key via web portal
2. **Check WiFi**: Ensure `wifiConnected = true` (visible on OLED)
3. **Check serial output**: Look for `[WU] Sending...` messages

### APRS Beacons Not Appearing

1. **Check callsign**: Verify APRS callsign format (e.g., `SQ8NA-15`)
2. **Check location**: Ensure latitude/longitude are set
3. **Check APRS-IS**: Confirm `rotate.aprs.net:14580` is reachable

### MQTT Data Not Arriving

1. **Check broker**: Verify broker address and port (default 1883)
2. **Check authentication**: Verify username/password if required
3. **Check topic**: Ensure you're subscribing to correct MQTT topic

## FAQs

**Q: Can I use TEST_WEATHER with a real sensor connected?**
A: Yes, but TEST_WEATHER will bypass sensor reading and always use emulated data.

**Q: Does TEST_WEATHER affect power consumption?**
A: Slightly less (radio not actively receiving), but WiFi/transmission overhead remains.

**Q: Can I modify weather data in real-time?**
A: Not without recompiling. The values are hardcoded in firmware. For dynamic testing, use MQTT/WebSocket integration.

**Q: Why is sensor initialization still called in TEST_WEATHER mode?**
A: Because some boards require initialization for stability. It's safe to initialize even if not used.
