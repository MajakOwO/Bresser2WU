# API Reference

Technical reference for the main classes and functions in Bresser Weather Sensor to WU + APRS-IS Gateway project.

## Table of Contents

1. [WeatherSensor API](#weathersensor-api)
2. [APRS API](#aprs-api)
3. [Configuration API](#configuration-api)
4. [Utility Functions](#utility-functions)

---

## WeatherSensor API

### Class: `WeatherSensor` (from BresserWeatherSensorReceiver library)

Main class for receiving and decoding Bresser wireless sensor messages.

### Constructor

```cpp
WeatherSensor()
```

Creates a new WeatherSensor instance.

### Methods

#### `void begin(const char* configName, bool verbose = false, uint8_t numSensors = 1)`

Initialize the sensor receiver.

**Parameters:**
- `configName` (const char*): Configuration name
- `verbose` (bool): Enable debug output (default: false)
- `numSensors` (uint8_t): Number of sensors to track (default: 1)

**Returns:** void

**Example:**
```cpp
WeatherSensor ws;
ws.begin("BresserWX", true, 1);  // Track 1 sensor with verbose output
```

---

#### `bool update()`

Check for new sensor data from the radio receiver.

Call this frequently (every 100ms) in the main loop.

**Returns:** 
- `true` if new data received
- `false` if no new data

**Example:**
```cpp
if(ws.update()) {
    // New sensor data available
    float temp = ws.sensor[0].w.temp_c;
}
```

---

#### `uint8_t getSensorCount()`

Get number of currently tracked sensors.

**Returns:** Number of active sensors (0 to numSensors)

**Example:**
```cpp
uint8_t count = ws.getSensorCount();
Serial.printf("Sensors found: %u\n", count);
```

---

#### `bool getData(uint8_t sensorIndex, sensor_t& data)`

Get decoded sensor data for a specific sensor.

**Parameters:**
- `sensorIndex` (uint8_t): Sensor array index (0 to numSensors-1)
- `data` (sensor_t&): Output buffer for sensor data

**Returns:**
- `true` if data available
- `false` if sensor not found

**Example:**
```cpp
sensor_t sensorData;
if(ws.getData(0, sensorData)) {
    Serial.printf("Temp: %.1f°C\n", sensorData.w.temp_c);
}
```

---

### Sensor Data Structure

The `sensor_t` structure contains decoded weather data:

```cpp
struct sensor_t {
    uint32_t   id;              // Sensor ID (unique identifier)
    uint32_t   lastUpdate;      // Timestamp of last update (ms)
    bool       complete;        // Data complete/valid
    
    // Weather data
    struct {
        float   temp_c;         // Temperature (Celsius)
        uint8_t humidity;       // Humidity (0-100%)
        float   wind_speed_ms;  // Wind speed (m/s)
        float   wind_gust_ms;   // Wind gust (m/s)
        uint16_t wind_dir;      // Wind direction (0-359 degrees)
        float   rain_mm;        // Rainfall (mm)
        
        // Optional (Bresser 7-in-1)
        float   uv;             // UV index
        float   solar_rad;      // Solar radiation (W/m²)
        
        // Status flags
        bool    temp_ok;        // Temperature valid
        bool    humidity_ok;    // Humidity valid
        bool    wind_ok;        // Wind data valid
        bool    rain_ok;        // Rain data valid
        bool    battery_ok;     // Battery status OK
        int8_t  rssi;           // RSSI (dBm)
    } w;
};
```

**Example usage:**
```cpp
Serial.printf("Sensor ID: 0x%08X\n", ws.sensor[0].id);
Serial.printf("Temperature: %.1f°C\n", ws.sensor[0].w.temp_c);
Serial.printf("Humidity: %u%%\n", ws.sensor[0].w.humidity);
Serial.printf("Wind: %.1f m/s @ %u°\n", 
              ws.sensor[0].w.wind_speed_ms, 
              ws.sensor[0].w.wind_dir);
Serial.printf("Rain: %.1f mm\n", ws.sensor[0].w.rain_mm);
Serial.printf("Battery: %s\n", ws.sensor[0].w.battery_ok ? "OK" : "LOW");
```

---

## APRS API

### Class: `APRS` (in src/APRS.h/.cpp)

TCP client for APRS-IS network communication.

### Constructor

```cpp
APRS()
```

Creates a new APRS-IS client instance.

### Methods

#### `bool begin(const char* callsign, const char* password, float latitude, float longitude, const char* comment = "")`

Initialize APRS client with station information.

**Parameters:**
- `callsign` (const char*): Amateur radio callsign with SSID (e.g., "SQ9ABC-13")
- `password` (const char*): APRS-IS authentication code (5 digits)
- `latitude` (float): Station latitude in decimal degrees (e.g., 52.2297)
- `longitude` (float): Station longitude in decimal degrees (e.g., 21.0122)
- `comment` (const char*): Optional comment text (default: "")

**Returns:** `true` if initialization successful

**Example:**
```cpp
APRS aprs;
aprs.begin("SQ9ABC-13", "12345", 52.2297, 21.0122, "Weather Station");
```

---

#### `bool isConnected()`

Check if currently connected to APRS-IS server.

**Returns:** 
- `true` if connected
- `false` if disconnected

**Example:**
```cpp
if(aprs.isConnected()) {
    // Safe to send data
} else {
    // Will attempt to reconnect on next send
}
```

---

#### `bool connectToServer()`

Establish connection to APRS-IS server.

Automatically called by sendWeatherData() if not connected.

**Returns:**
- `true` if connection successful
- `false` if connection failed

**Example:**
```cpp
if(!aprs.isConnected()) {
    aprs.connectToServer();
}
```

---

#### `bool sendWeatherData(float temp_f, uint8_t humidity, float wind_speed_mph, float wind_gust_mph, uint16_t wind_dir, float rain_in, float pressure_inhg, bool battery_ok, int8_t rssi_dbm)`

Send weather data to APRS-IS network.

**Parameters:**
- `temp_f` (float): Temperature in Fahrenheit
- `humidity` (uint8_t): Humidity 0-100%
- `wind_speed_mph` (float): Wind speed in mph
- `wind_gust_mph` (float): Wind gust in mph
- `wind_dir` (uint16_t): Wind direction 0-359 degrees
- `rain_in` (float): Rainfall in inches
- `pressure_inhg` (float): Barometric pressure in inHg
- `battery_ok` (bool): Battery status (true=OK, false=Low)
- `rssi_dbm` (int8_t): Signal strength in dBm

**Returns:**
- `true` if data sent successfully
- `false` if send failed (will attempt reconnect)

**Example:**
```cpp
aprs.sendWeatherData(
    72.5,      // Temperature (°F)
    65,        // Humidity (%)
    5.2,       // Wind speed (mph)
    8.3,       // Gust (mph)
    180,       // Direction (degrees)
    0.05,      // Rain (inches)
    29.92,     // Pressure (inHg)
    true,      // Battery OK
    -85        // RSSI (dBm)
);
```

---

#### `bool sendStatus(const char* status_text)`

Send status message to APRS network.

**Parameters:**
- `status_text` (const char*): Status message (max 50 chars)

**Returns:**
- `true` if sent successfully
- `false` if send failed

**Example:**
```cpp
aprs.sendStatus("Weather Station Online");
```

---

## Configuration API

### Class: `Preferences` (ESP32 built-in)

Used for storing configuration in NVS (Non-Volatile Storage).

### Methods

#### `void begin(const char* namespace, bool readOnly = false)`

Open the Preferences namespace.

**Parameters:**
- `namespace` (const char*): Namespace name (e.g., "BresserWX")
- `readOnly` (bool): Open in read-only mode (default: false)

**Example:**
```cpp
Preferences prefs;
prefs.begin("BresserWX", false);
```

---

#### `String getString(const char* key, const String& defaultValue = String())`

Read string from preferences.

**Parameters:**
- `key` (const char*): Key name
- `defaultValue` (const String&): Default if not found

**Returns:** String value

**Example:**
```cpp
String wuID = prefs.getString("wuID", "");
```

---

#### `void putString(const char* key, const String& value)`

Write string to preferences.

**Parameters:**
- `key` (const char*): Key name
- `value` (const String&): Value to store

**Example:**
```cpp
prefs.putString("wuID", "KZZXY50");
```

---

#### `void clear()`

Clear all keys in namespace.

**Example:**
```cpp
prefs.clear();  // Erase all configuration
```

---

#### `void end()`

Close the Preferences namespace.

**Example:**
```cpp
prefs.end();  // Done with preferences
```

---

### Stored Configuration Keys

| Key | Type | Example | Purpose |
|-----|------|---------|---------|
| `wuID` | String | `KZZXY50` | Weather Underground Station ID |
| `wuKey` | String | `a1b2c3d4...` | Weather Underground API Key |
| `aprsCall` | String | `SQ9ABC-13` | APRS-IS Callsign |
| `aprsPass` | String | `12345` | APRS-IS Passcode |
| `aprsLat` | String | `52.2297` | Station Latitude |
| `aprsLon` | String | `21.0122` | Station Longitude |
| `aprsComment` | String | `Weather Station` | Optional APRS Comment |
| `rainMid` | Float | `12.5` | Daily Rain Counter Reference |

---

## Utility Functions

### Temperature Conversion

```cpp
// Celsius to Fahrenheit
float tempF = (tempC * 9.0 / 5.0) + 32.0;

// Fahrenheit to Celsius
float tempC = (tempF - 32.0) * 5.0 / 9.0;
```

### Wind Speed Conversion

```cpp
// m/s to mph
float windMph = windMs * 2.237;

// mph to m/s
float windMs = windMph / 2.237;

// m/s to km/h
float windKmh = windMs * 3.6;
```

### Pressure Conversion

```cpp
// hPa to inHg
float pressureInhg = pressureHpa * 0.02953;

// inHg to hPa
float pressureHpa = pressureInhg / 0.02953;
```

### Rainfall Conversion

```cpp
// mm to inches
float rainInches = rainMm * 0.039370;

// inches to mm
float rainMm = rainInches / 0.039370;
```

---

## Main Application Loop Pattern

```cpp
// Global objects
WeatherSensor ws;
APRS aprs;
Preferences prefs;
WiFiClient client;

// Timers
uint32_t lastWUUpdate = 0;
uint32_t lastAPRSUpdate = 0;

void setup() {
    // Initialize sensors and services
    ws.begin("BresserWX", true, 1);
    aprs.begin("SQ9ABC-13", "12345", 52.2297, 21.0122, "Weather");
    
    prefs.begin("BresserWX");
    String wuID = prefs.getString("wuID", "");
    prefs.end();
}

void loop() {
    // Check for new sensor data
    if(ws.update()) {
        // Process weather data
        float tempF = (ws.sensor[0].w.temp_c * 9.0 / 5.0) + 32.0;
        uint8_t humidity = ws.sensor[0].w.humidity;
        
        // Weather Underground (every 15 sec)
        if(millis() - lastWUUpdate > 15000) {
            sendToWU(tempF, humidity);  // Your function
            lastWUUpdate = millis();
        }
        
        // APRS-IS (every 10 min)
        if(millis() - lastAPRSUpdate > 600000) {
            aprs.sendWeatherData(tempF, humidity, /* ... */);
            lastAPRSUpdate = millis();
        }
    }
    
    delay(100);  // Check sensors every 100ms
}
```

---

## External Library References

- **RadioLib**: https://jgromes.github.io/RadioLib/
- **WiFiManager**: https://github.com/tzapu/WiFiManager
- **Adafruit BMP280**: https://github.com/adafruit/Adafruit_BMP280_Library
- **ArduinoJson**: https://arduinojson.org/

---

## Version History

- **v1.0.0+**: Initial API reference
- **v1.1.0+**: Added configuration API section

---

For implementation examples, see [src/main.cpp](../src/main.cpp) and [src/APRS.cpp](../src/APRS.cpp).
