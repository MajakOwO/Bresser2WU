# Project Architecture

This document describes the overall architecture of Bresser Weather Sensor to WU + APRS-IS Gateway.

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    Bresser 7-in-1 Weather Sensor               │
│                           (868 MHz RF)                          │
└────────────────────────────┬────────────────────────────────────┘
                             │ Wireless
                             ↓
┌─────────────────────────────────────────────────────────────────┐
│                  Radio Module (SX1262/SX1276)                  │
│              Connected via SPI to ESP32 GPIO Pins               │
└────────────────────────────┬────────────────────────────────────┘
                             │ SPI
                             ↓
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32 Microcontroller                    │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ main.cpp (Application Logic)                             │  │
│  │  - Sensor data processing                                │  │
│  │  - Weather calculations                                  │  │
│  │  - Rain gauge history                                    │  │
│  │  - Dual-service transmission                             │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ WiFiManager Module                                       │  │
│  │  - Configuration portal (192.168.4.1)                    │  │
│  │  - Stores credentials in NVS (non-volatile storage)      │  │
│  │  - Manages WiFi connection                               │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ APRS.h/.cpp (TCP to APRS-IS)                             │  │
│  │  - TCP client connection                                 │  │
│  │  - APRS packet formatting                                │  │
│  │  - Weather data transmission (10 min)                    │  │
│  │  - Position beaconing                                    │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ WeatherSensor (Library)                                  │  │
│  │  - Bresser 5/6/7-in-1 decoding                           │  │
│  │  - Multi-sensor support                                  │  │
│  │  - RadioLib wrapper                                      │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Optional: BMP280 Pressure Sensor (I2C)                   │  │
│  │  - Reads barometric pressure                             │  │
│  │  - Complements Bresser data                              │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────┬──────────────────────────────┬──────────────────────────┬─┘
      │ WiFi                         │ WiFi                     │
      ↓ (15 sec)                     ↓ (10 min)                 │
┌────────────────────┐      ┌──────────────────┐                │
│ Weather Underground│      │    APRS-IS       │                │
│                    │      │   rotate.aprs.net│                │
│ https://           │      │   :14580 (TCP)   │                │
│ wunderground.com   │      │                  │                │
│                    │      │ → aprs.fi        │                │
└────────────────────┘      └──────────────────┘                │
                                                                  │
                            │ Optional (Debug/Serial)           │
                            ↓                                     │
                        ┌────────────────┐                        │
                        │  Serial Monitor│                        │
                        │  (115200 baud) │                        │
                        └────────────────┘                        │
```

## Core Components

### 1. **main.cpp** - Application Controller
- **Purpose**: Central orchestration of all components
- **Key Functions**:
  - `setup()` - Initialization
  - `loop()` - Main execution loop
  - `setupPreferences()` - Load/save configuration from NVS
  - `setupWiFiManager()` - Configure WiFi and credentials
  - `setupSensors()` - Initialize WeatherSensor and BMP280
  - `setupAPRS()` - Initialize APRS client
  - `calculateWeatherData()` - Convert units and calculations
  - `sendToWU()` - HTTP POST to Weather Underground
  - `sendToAPRS()` - TCP transmission to APRS-IS
  - `updateRainHistory()` - Maintain 60-minute rain buffer
  - `updateRainCounters()` - Track midnight rollover

### 2. **APRS.h / APRS.cpp** - APRS-IS Client
- **Purpose**: TCP communication with APRS-IS servers
- **Key Features**:
  - Maintains persistent TCP connection to APRS-IS
  - Formats weather data in APRS format
  - Handles authentication with callsign/password
  - Supports position beaconing with weather telemetry
  - Automatic reconnection with 5-minute retry interval
- **Key Methods**:
  - `begin()` - Initialize with callsign, password, coordinates
  - `connectToServer()` - Establish TCP connection
  - `sendWeatherData()` - Transmit formatted weather data
  - `sendStatus()` - Send status text
  - `isConnected()` - Check connection state

### 3. **WeatherSensor** - Sensor Receiver (Library)
- **Source**: BresserWeatherSensorReceiver (external library)
- **Purpose**: Decode Bresser wireless sensor messages (868 MHz)
- **Supported Sensors**:
  - Bresser 5-in-1 (temperature, humidity, wind, rain)
  - Bresser 6-in-1 (+ wind direction)
  - Bresser 7-in-1 (+ solar radiation, UV)
- **Data Structure**: Multi-sensor support (up to 8 sensors)
- **Key Data**:
  - Temperature, humidity, dew point
  - Wind speed, gust, direction
  - Rainfall (mm)
  - Battery status, signal strength (RSSI)

### 4. **InitBoard.h / InitBoard.cpp** - Hardware Initialization
- **Purpose**: Board-specific GPIO and peripheral setup
- **Supports**: 20+ ESP32 board variants
- **Initialization**:
  - GPIO pin configuration
  - Serial port setup
  - Radio module initialization
  - I2C for BMP280 (if enabled)

### 5. **BMP280** - Optional Pressure Sensor
- **Interface**: I2C (default GPIO 21=SDA, GPIO 22=SCL)
- **Data**: Barometric pressure, temperature, altitude
- **Usage**: Complements Bresser sensor data
- **Configuration**: In `src/WeatherSensorCfg.h`

## Data Flow

### Reception Cycle
```
1. Radio Module receives 868 MHz signal from Bresser sensor
   ↓
2. WeatherSensor.update() decodes message
   ↓
3. Data stored in WeatherSensor.sensor[] array
   ↓
4. Data is valid (timestamp, checksums verified)
```

### Processing & Transmission Cycle
```
main.loop() executes every ~100ms:
   ↓
1. Check for new sensor data (WeatherSensor.update())
   ↓
2. If data available:
     - Calculate derived values (dew point, unit conversions)
     - Update rain gauge history (60-minute buffer)
     - Check midnight rollover for daily rain
   ↓
3. Timer: Every 15 seconds → sendToWU()
     - Format HTTP POST with weather data
     - Send to Weather Underground API
     - Handle response (retry on failure)
   ↓
4. Timer: Every 10 minutes → sendToAPRS()
     - Ensure APRS connection (reconnect if needed)
     - Format APRS packet with weather data
     - Send to APRS-IS server
```

### Configuration Storage
```
Preferences (NVS - Non-Volatile Storage):
  ├── wuID          → Weather Underground Station ID
  ├── wuKey         → Weather Underground API Key
  ├── aprsCall      → APRS Callsign (e.g., SQ9ABC-13)
  ├── aprsPass      → APRS Password
  ├── aprsLat       → Latitude (decimal)
  ├── aprsLon       → Longitude (decimal)
  ├── aprsComment   → Optional APRS comment
  └── rainMid       → Daily rain counter (midnight reference)
```

## Key Algorithms

### 1. Dew Point Calculation (Magnus Formula)
```cpp
dewpoint = temperature - (100 - humidity) / 5
```
Used for atmospheric analysis in APRS telemetry.

### 2. Rain Gauge History (60-minute circular buffer)
```
rainHistory[60] array stores 1 minute samples
- Circular buffer: when full, overwrites oldest entry
- Hourly total = sum of last 60 samples
- Daily total = sum from midnight reference
- Midnight detection: if current_rain < midnight_reference
```

### 3. Unit Conversions
```
Temperature:  °C → °F = (°C × 9/5) + 32
Wind Speed:   m/s → mph = m/s × 2.237
Pressure:     hPa → inHg = hPa × 0.02953
```

## Communication Protocols

### Weather Underground (HTTP/REST)
- **Endpoint**: `https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php`
- **Method**: GET with query parameters
- **Frequency**: Every 15 seconds
- **Timeout**: 10 seconds
- **Retry**: None (continues on next cycle)
- **Parameters**: Station ID, API Key, temperature, humidity, wind, rain, pressure

### APRS-IS (TCP/Radio)
- **Server**: `rotate.aprs.net:14580`
- **Protocol**: APRS-IS TCP with authentication
- **Frequency**: Every 10 minutes (also used for weather telemetry)
- **Format**: APRS packet with:
  - Position (latitude/longitude in APRS format)
  - Weather telemetry (temp, wind, rain, pressure)
  - Battery status in position report
  - Comment with device info and link
- **Auto-reconnect**: Every 5 minutes if disconnected

## WiFiManager Configuration

```
Initial Boot Sequence:
  1. ESP32 starts as Access Point "BresserWX"
  2. User connects to AP from phone/computer
  3. Browser opens to 192.168.4.1 automatically (captive portal)
  4. WiFiManager shows configuration form
  5. User enters:
     - Target WiFi SSID & password
     - Weather Underground credentials
     - APRS credentials and position
  6. Device saves to NVS and reboots
  7. Device connects to target WiFi network
  
Subsequent Boots:
  1. Load credentials from NVS
  2. Attempt to connect to saved WiFi
  3. If fails after 300 seconds: restart WiFiManager
```

## Timing Architecture

```
Main Loop Timing (~100ms cycle):
  ├─ 0ms:   WeatherSensor.update() - check for new radio data
  ├─ 10ms:  Process received data
  ├─ 50ms:  Check timers (WU & APRS)
  ├─ 80ms:  Serial debugging (if enabled)
  └─ 100ms: Loop complete, restart

Transmission Timers:
  ├─ WU_UPDATE_INTERVAL:   15,000 ms (15 seconds)
  └─ APRS_UPDATE_INTERVAL: 600,000 ms (10 minutes)

Connection Timeouts:
  ├─ HTTP request timeout: 10 seconds
  ├─ WiFi autoConnect timeout: 300 seconds
  └─ APRS reconnect retry: 300 seconds (5 minutes)
```

## Error Handling

### WiFi Connection Loss
```
→ sendToWU() returns false
→ HTTP client timeout (10s)
→ Retry on next 15s cycle
→ No data loss (continues collecting sensor data)
```

### APRS Connection Loss
```
→ APRS::isConnected() returns false
→ Attempt reconnection
→ If fails: retry every 5 minutes
→ No data loss (continues collecting sensor data)
```

### Sensor Malfunction
```
→ ws.sensor[i].w.temp_ok = false
→ Skip transmission cycle
→ Continue checking for recovery
→ Log to serial output
```

## Memory Usage

```
ESP32 SRAM (estimated):
  ├─ Stack: ~2 KB
  ├─ Heap (WiFi, HTTP): ~50 KB
  ├─ rainHistory[60]: ~240 bytes
  ├─ Configuration strings: ~500 bytes
  ├─ APRS buffer: ~1 KB
  └─ Total available: 160 KB+ (very safe margin)

Flash Storage (NVS):
  ├─ Configuration keys: ~500 bytes
  └─ Total: 4096 byte partition (plenty available)
```

## Dependencies & Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| **BresserWeatherSensorReceiver** | Latest | Sensor decoding |
| **RadioLib** | 7.6.0+ | 868 MHz radio driver |
| **WiFiManager** | Latest | WiFi + configuration |
| **Adafruit BMP280** | Latest | Pressure sensor (optional) |
| **Arduino Framework** | - | ESP32 core |

## Threading & Concurrency

- **Single-threaded**: No RTOS tasks used
- **Blocking calls**: Minimized
  - HTTP requests: 10s timeout maximum
  - WiFiManager: 300s timeout
  - Serial I/O: Non-blocking
- **Interrupt handling**: Managed by RadioLib internally
- **Safe for**: 24/7 continuous operation

## Future Architecture Improvements

Potential enhancements:

1. **MQTT Support** - Send to home automation systems
2. **Local Data Logging** - SD card or SPIFFS storage
3. **Web Dashboard** - Real-time local web interface
4. **Multiple Sensors** - Receive from multiple Bresser devices simultaneously (already supported by WeatherSensor library)
5. **Advanced Filtering** - Kalman filter for wind speed smoothing
6. **Lightning Detection** - Support for Bresser lightning sensor
7. **Wireless Updates** - OTA firmware updates

---

For detailed implementation, see [src/main.cpp](../src/main.cpp), [src/APRS.h/cpp](../src/APRS.h), and [src/WeatherSensorCfg.h](../src/WeatherSensorCfg.h).
