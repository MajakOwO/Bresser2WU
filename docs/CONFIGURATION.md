# Configuration Guide

Detailed reference for all configuration options for Bresser Weather Sensor to WU + APRS-IS Gateway.

## Configuration Methods

Configuration is stored in **ESP32 NVS (Non-Volatile Storage)** and persists across reboots. Two ways to configure:

### 1. WiFiManager Web Portal (Recommended - Runtime)
- Easiest method
- No code changes required
- Works after device boots
- Access via: `192.168.4.1` (if WiFi fails) or `192.168.1.100/wifisettings` (web portal)

### 2. Direct Code Modification (Development)
- Edit `src/WeatherSensorCfg.h` and `src/main.cpp`
- Requires recompilation and upload
- Use for permanent hardware changes

## WiFiManager Configuration Portal

### Access the Portal

**First Time (no WiFi saved):**
1. Device starts as Access Point
2. Connect to WiFi: **`BresserWX`** (no password)
3. Open browser: **`192.168.4.1`**
4. Captive portal automatically appears (or click "Sign In")

**After WiFi Connected:**
1. Open browser to ESP32's IP address (see serial monitor)
2. Go to: **`http://<ESP32_IP>/wifisettings`**
3. Enter WiFiManager password (default: check code)

### Configuration Parameters

#### WiFi Settings
```
SSID:     Your WiFi network name (2.4 GHz recommended)
Password: WiFi password (max 64 characters)
Hostname: Device hostname (e.g., "BresserWX") - optional
```

#### Weather Underground Reporting
```
Station ID:  Unique ID (e.g., "KIZZMY" + "50")
             Get from: https://www.wunderground.com/weather/api
             Format:   CCSSXYYY or CCSSXYYY-N
             Where: CC=country, SS=state, X=area, YYY=number, N=suffix

API Key:     Personal API key from Weather Underground
             Generate at: https://www.wunderground.com/weather/api
             Keep secret - only shared with WU servers
             
Reporting:   Automatic every 15 seconds (hardcoded)
             Posts to: rtupdate.wunderground.com
```

#### APRS-IS Configuration
```
Callsign:    Amateur radio call with SSID (e.g., "SQ9ABC-13")
             Format: CALL-SSID where SSID=0-15 (15=highest)
             Without SSID, defaults to -0
             
Passcode:    APRS authentication code (NOT your radio password!)
             Generate: https://www.heywhatsthat.com/aprs_passcode.html
             Enter your callsign there, get 5-digit code
             Keep secret - identifies your station on APRS-IS

Latitude:    Decimal degrees (e.g., "52.2297" for Warsaw)
             Negative = South, Positive = North
             Range: -90 to +90
             Format: DD.DDDD (4+ decimal places for accuracy)

Longitude:   Decimal degrees (e.g., "21.0122" for Warsaw)
             Negative = West, Positive = East
             Range: -180 to +180
             Format: DD.DDDD (4+ decimal places for accuracy)
             
Comment:     Optional text for APRS packet (max 43 chars)
             Examples: "Weather Station", "QTH: Garden", "Temp+Wind"
             Appears on aprs.fi and ADS-B Exchange
```

### Saving Configuration

1. Fill all required fields
2. Click **Save**
3. Device will:
   - Validate inputs
   - Save to NVS
   - Reboot
   - Connect to WiFi
   - Start transmitting

### Resetting Configuration

**Option 1: WiFiManager Reset Portal**
1. Access WiFiManager portal (`192.168.4.1`)
2. Click "Reset settings"
3. Device will erase configuration and reboot

**Option 2: Hard Reset (code)**
1. Edit `src/main.cpp`
2. Uncomment: `wm.resetSettings();` in `setupWiFiManager()`
3. Upload and run once
4. Comment it back out and reupload

## Compile-Time Configuration

### src/WeatherSensorCfg.h

Hardware configuration that requires recompilation to change:

#### Radio Module Selection (for generic ESP32)

```cpp
#elif defined(ESP32)
    // Choose ONE:
    #define ESP32_VARIANT_SX1262    // ← Recommended
    // #define ESP32_VARIANT_SX1276
    // #define ESP32_VARIANT_CC1101
    // #define ESP32_VARIANT_LR1121
```

#### GPIO Pin Configuration

Each variant defines pins. Example for SX1262:

```cpp
#if defined(ESP32_VARIANT_SX1262)
    #define USE_SX1262              // Enable SX1262 driver
    
    // SPI Communication
    #define PIN_RECEIVER_SCK    18  // SCK - can't change (hardware SPI)
    #define PIN_RECEIVER_MOSI   23  // MOSI - can't change (hardware SPI)
    #define PIN_RECEIVER_MISO   19  // MISO - can't change (hardware SPI)
    #define PIN_RECEIVER_CS     27  // Chip Select - flexible
    
    // Interrupt & Status
    #define PIN_RECEIVER_IRQ    21  // DIO1/Interrupt - flexible
    #define PIN_RECEIVER_GPIO   33  // Busy/GPIO - flexible
    
    // Control Signals
    #define PIN_RECEIVER_RST    32  // Reset - flexible
```

**Note**: SPI pins (SCK, MOSI, MISO) are hardware-assigned. Only CS, IRQ, GPIO, RST can change.

#### I2C BMP280 Configuration

```cpp
#define USE_BMP280                  // Enable BMP280 sensor
#define PIN_SDA     21              // I2C Data (SDA)
#define PIN_SCL     22              // I2C Clock (SCL)
#define BMP280_ADDR 0x76            // I2C address (0x76 or 0x77)
#define BMP280_MODE BMX280_SLEEP_MODE  // Sleep/Normal/Forced
```

#### Board Selection

```cpp
// Uncomment your board:
#define ARDUINO_HELTEC_WIFI_LORA_32_V3  // Heltec Wireless Stick Lite V3
#define TTGO_LORA32_V2_1_20180221        // TTGO LoRa32 V2.1
// ... etc - see file for all options
```

### src/main.cpp

Application-level settings (search for these constants):

```cpp
// Sensor Update Intervals
#define SENSOR_CHECK_INTERVAL    100    // ms - check for new sensor data

// Service Update Intervals
#define WU_UPDATE_INTERVAL       15000  // ms (15 seconds)
#define APRS_UPDATE_INTERVAL     600000 // ms (10 minutes)

// WiFi Timeouts
#define WIFI_AUTOCONNECT_TIMEOUT 300    // seconds (5 minutes)

// APRS Reconnection
#define APRS_RECONNECT_INTERVAL  300000 // ms (5 minutes)

// Rain Gauge Buffer
#define RAIN_HISTORY_SIZE        60     // 1-minute samples for 60-minute history

// Serial Debug Output
#define ENABLE_DEBUG_OUTPUT      1      // 1 = enabled, 0 = disabled
```

To modify:
1. Edit value in `src/main.cpp`
2. Rebuild: `pio run`
3. Upload: `pio run -t upload`

## Data Transmission Configuration

### Weather Underground Settings

**Endpoint**: `https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php`

**Query Parameters Sent**:
```
action=updateraw
ID=<Station ID>
PASSWORD=<API Key>
dateutc=now
tempf=<temperature in °F>
humidity=<0-100>
windspeedmph=<wind speed>
windgustmph=<gust speed>
winddir=<0-360 degrees>

# Rainfall:
# - rain in inches/hour is sent as precipratein (computed in firmware)
# - daily rain remains sent as dailyrainin
precipratein=<mm/h -> in/h converted to inches per hour>
dailyrainin=<daily rain in inches>

baromin=<pressure in inHg>  # If BMP280 available
rtfreq=<report frequency>
```

**Data Validation** (before sending):
- Temperature: -20°C to +60°C (valid range)
- Humidity: 0-100% (invalid if sensor reports error)
- Wind: 0-100 mph (invalid if sensor error)
- Rain: non-negative (resets at midnight)

### APRS-IS Settings

**Server**: `rotate.aprs.net:14580`

**Data Sent** (every 10 minutes + telemetry):

```
# Position Report Example:
SQ9ABC-13>APRS,qAS,SQ9ZZZ:/120530h5213.78N/02100.74E°234/055/A=00480
  Temperature: 019F Wind: 234@055 Gust: 068 Humidity: 56h 
  Barometer: 10183b eWX Station v1.0

# Components Explained:
SQ9ABC-13        = Your callsign-SSID
APRS,qAS,...     = APRS protocol header
120530h          = UTC time (12:05:30)
5213.78N         = Latitude (52°13.78'N)
02100.74E        = Longitude (021°00.74'E)
234              = Wind direction (magnetic degrees)
055              = Wind speed (mph)
068              = Gust (mph)
A=00480          = Altitude (feet)
T063/H56/b10183  = Temperature (°F) / Humidity / Barometer (0.1 mb)
eWX              = Device identifier
```

## Database Storage (NVS Preferences)

Stored in ESP32 flash memory (4 KB partition):

| Key | Size | Value | Type |
|-----|------|-------|------|
| `wuID` | 32 B | Station ID | String |
| `wuKey` | 32 B | API Key | String |
| `aprsCall` | 16 B | Callsign-SSID | String |
| `aprsPass` | 8 B | Passcode | String |
| `aprsLat` | 16 B | Latitude | String |
| `aprsLon` | 16 B | Longitude | String |
| `aprsComment` | 64 B | Optional comment | String |
| `rainMid` | 4 B | Daily rain reference | Float |

**Total Used**: ~188 bytes (plenty of space available)

**Access Code**:
```cpp
Preferences prefs;
prefs.begin("BresserWX");

// Read
String wuID = prefs.getString("wuID", "");

// Write
prefs.putString("wuID", "KZZXY50");

prefs.end();
```

## Advanced Configuration

### Rain Gauge Calibration

If rainfall readings seem wrong:

1. Check sensor battery (low battery affects accuracy)
2. Verify rain bucket tipping in rain (physically test)
3. Adjust in code (src/main.cpp):

```cpp
// Convert rain mm to inches (default)
// If your weather service prefers mm:
float rainInches = rainMM * 0.039370;  // ← Change multiplier here
```

### Temperature Offset

If temperature reads 2°C too high (common in sunny locations):

```cpp
// In src/main.cpp, before sending to WU:
float tempC = ws.sensor[i].w.temp_c - 2.0;  // Subtract offset
float tempF = tempC * 9.0 / 5.0 + 32.0;
```

### Wind Speed Filtering

To smooth out wind speed spikes (optional, advanced):

```cpp
// Simple rolling average
#define WIND_HISTORY_SIZE 10
float wind_history[WIND_HISTORY_SIZE];
int wind_idx = 0;

float smoothWind(float raw_speed) {
    wind_history[wind_idx] = raw_speed;
    wind_idx = (wind_idx + 1) % WIND_HISTORY_SIZE;
    
    float sum = 0;
    for(int i=0; i<WIND_HISTORY_SIZE; i++) {
        sum += wind_history[i];
    }
    return sum / WIND_HISTORY_SIZE;
}
```

### APRS Frequency Adjustment

Default: Every 10 minutes. To change:

```cpp
// In src/main.cpp
#define APRS_UPDATE_INTERVAL  300000  // 5 minutes (300000 ms)
// or
#define APRS_UPDATE_INTERVAL  600000  // 10 minutes (600000 ms) - default
// or
#define APRS_UPDATE_INTERVAL 1800000  // 30 minutes (1800000 ms)
```

### HTTP Timeout Extension

If Weather Underground requests timeout:

```cpp
// In src/main.cpp
http.setTimeout(20000);  // Increase from default (in milliseconds)
```

## Environment-Specific Configuration

### PlatformIO Environments

Different settings per environment possible. Example:

**platformio.ini**:
```ini
[env:esp32dev-sx1262]
build_flags =
    -D ENABLE_DEBUG_OUTPUT=1      # Verbose logging

[env:esp32dev-sx1276]
build_flags =
    -D ENABLE_DEBUG_OUTPUT=0      # Minimal logging
```

Then rebuild for specific environment:
```bash
pio run -e esp32dev-sx1262
```

## Configuration Backup & Restore

### Backup NVS Settings

To save configuration to file (for recovery):

```bash
# Using esptool (Python)
pip install esptool
esptool.py -p COM3 read_flash 0x9000 0x1000 nvs_backup.bin
```

### Restore NVS Settings

```bash
esptool.py -p COM3 write_flash 0x9000 nvs_backup.bin
```

## Troubleshooting Configuration

### Settings Not Saving

**Problem**: Config form shows but doesn't save
- **Solution 1**: Check serial monitor for error messages
- **Solution 2**: Try shorter SSID/password (special characters may cause issues)
- **Solution 3**: Full reset and reconfigure

### Settings Lost After Reboot

**Problem**: Configuration disappears after power cycle
- **Likely**: NVS corruption
- **Solution**:
```cpp
// In setup():
prefs.begin("BresserWX");
prefs.clear();  // Erase all
prefs.end();
// Then reconfigure via WiFiManager
```

### Can't Access WiFiManager Portal

**Problem**: 192.168.4.1 doesn't load
- **Check**: Device is powered and booted (see serial monitor)
- **Check**: Phone is connected to "BresserWX" WiFi
- **Try**: Manual IP: Use `192.168.4.1:80` or just `192.168.4.1`
- **Check**: Firewall not blocking port 80

## Configuration Limits

| Parameter | Min | Max | Notes |
|-----------|-----|-----|-------|
| SSID | 1 | 32 chars | WiFi network name |
| WiFi Password | 8 | 63 chars | WPA2 minimum |
| Station ID | 4 | 20 chars | Weather Underground ID |
| API Key | 32 | 64 chars | Typically 32 chars |
| Callsign | 3 | 9 chars | CALL-SSID format |
| APRS Comment | 0 | 43 chars | Optional |
| Latitude | -90 | +90 | Decimal degrees |
| Longitude | -180 | +180 | Decimal degrees |

---

For more help, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md) and [ARCHITECTURE.md](ARCHITECTURE.md).
