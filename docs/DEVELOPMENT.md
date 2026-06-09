# Development Guide

Guide for developers who want to understand, modify, or extend the Bresser Weather Sensor to WU + APRS-IS Gateway project.

## Project Structure

```
Bresser2WU/
├── .github/
│   └── workflows/
│       └── release-binaries.yml    # CI/CD for automated builds
│
├── docs/
│   ├── ARCHITECTURE.md             # System design
│   ├── INSTALLATION.md             # Setup guide
│   ├── CONFIGURATION.md            # Settings reference
│   ├── TROUBLESHOOTING.md          # Problem solving
│   ├── BOARD_VARIANTS.md           # Board support list
│   ├── RADIO_MODULES.md            # Radio module guide
│   ├── APRS_CONFIGURATION.md       # APRS detailed setup
│   └── README                      # Docs index
│
├── extras/
│   ├── hw_test/                    # Hardware testing utilities
│   ├── Python-Sensirion-SPS30/    # PM2.5 sensor library
│   └── logs/                       # Serial monitor logs
│
├── include/
│   └── README                      # Include directory (Arduino)
│
├── lib/
│   ├── README                      # Library management
│   └── BresserWeatherSensorReceiver/  # Main sensor library
│       └── src/
│           ├── InitBoard.h/cpp     # Hardware initialization
│           ├── Lightning.h/cpp     # Lightning detection
│           ├── RainGauge.h/cpp     # Rain measurement
│           ├── RollingCounter.h/cpp  # Counter utility
│           ├── WeatherSensor.h/cpp # Main sensor decoder
│           ├── WeatherSensorCfg.h  # Hardware config (KEY!)
│           ├── WeatherSensorConfig.cpp  # Config loading
│           ├── WeatherSensorDecoders.cpp # Protocol decoders
│           └── WeatherUtils.h/cpp  # Utility functions
│
├── src/
│   ├── main.cpp                    # Application logic (KEY!)
│   ├── APRS.h / APRS.cpp          # APRS-IS client (KEY!)
│   ├── config.h                    # Application config
│   ├── InitBoard.h / InitBoard.cpp # Board init
│   ├── Lightning.h / Lightning.cpp # Lightning support
│   ├── RainGauge.h / RainGauge.cpp # Rain measurement
│   ├── RollingCounter.h / RollingCounter.cpp
│   ├── WeatherSensor.h / WeatherSensor.cpp
│   ├── WeatherSensorCfg.h         # Hardware config (KEY!)
│   ├── WeatherSensorConfig.cpp    # Config mgmt
│   ├── WeatherSensorDecoders.cpp  # Protocol decode
│   └── WeatherUtils.h / WeatherUtils.cpp
│
├── test/
│   ├── Makefile                   # Test build system
│   ├── MakefileWorker.mk
│   ├── MakefileWorkerOverrides.mk
│   ├── README.md                  # Test documentation
│   │
│   ├── mocks/
│   │   ├── Arduino.h              # Arduino mock
│   │   ├── Preferences.h          # ESP32 mock
│   │   ├── WStringMock.h/cpp      # String mock
│   │   └── log_w_mock.h           # Log mock
│   │
│   ├── header_overrides/
│   │   └── ...                    # Override system headers
│   │
│   └── src/
│       ├── AllTests.cpp           # Test runner
│       ├── TestLightning.cpp      # Lightning tests
│       ├── TestRainGauge.cpp      # Rain gauge tests
│       ├── TestRainGaugeReal.cpp  # Integration tests
│       ├── TestRollingCounter.cpp # Counter tests
│       └── TestWeatherUtils.cpp   # Utils tests
│
├── platformio.ini                 # Build configuration
├── library.properties             # Library metadata
├── Doxyfile                       # Documentation generation
├── BUILD.md                       # Build instructions
├── CHANGELOG.md                   # Version history
├── README.md                      # Main documentation
└── copilot-instructions.md        # AI assistant rules
```

## Key Files to Understand

### 1. **src/main.cpp** - Application Core
- **Size**: ~500-700 lines
- **Purpose**: Main application logic, sensors, transmission
- **Key Functions**:
  - `setup()` - Initialization
  - `loop()` - Main event loop
  - `calculateWeatherData()` - Data processing
  - `sendToWU()` - Weather Underground submission
  - `sendToAPRS()` - APRS-IS beacon

**When to Edit**:
- Change transmission intervals
- Add new service (e.g., MQTT)
- Modify data calculations
- Add debug output

---

### 2. **src/WeatherSensorCfg.h** - Hardware Configuration
- **Size**: ~200-300 lines
- **Purpose**: GPIO pins, board selection, radio module choice
- **Key Defines**:
  - `ESP32_VARIANT_*` - Select radio module
  - `PIN_RECEIVER_*` - SPI/control pins
  - `PIN_SDA`, `PIN_SCL` - I2C pins
  - `USE_BMP280` - Optional sensor

**When to Edit**:
- Add support for new board variant
- Change GPIO pins
- Enable/disable BMP280
- Modify radio module

---

### 3. **src/APRS.h / APRS.cpp** - APRS-IS Client
- **Size**: ~300-400 lines
- **Purpose**: TCP communication with APRS-IS servers
- **Key Methods**:
  - `begin()` - Initialize with credentials
  - `connectToServer()` - Establish TCP
  - `sendWeatherData()` - Transmit beacon
  - `isConnected()` - Check status

**When to Edit**:
- Change APRS server
- Modify packet format
- Add new telemetry fields
- Change connection timeout

---

### 4. **lib/BresserWeatherSensorReceiver/src/WeatherSensor.h**
- **Purpose**: Sensor decoding library (external dependency)
- **Key Classes**: `WeatherSensor`
- **Key Methods**: `update()`, `getData()`, `getSensorCount()`

**When to Edit**: Usually don't - this is an external library. Modify your code to use its API instead.

---

## Development Workflow

### Setting Up Development Environment

1. **Clone and Open**
   ```bash
   git clone https://github.com/MajakOwO/Bresser2WU.git
   cd Bresser2WU
   code .  # Open in VS Code
   ```

2. **Install Extensions** (in VS Code):
   - PlatformIO IDE
   - C/C++ IntelliSense
   - Git Graph (optional)

3. **Build**
   ```bash
   pio run              # Build default env
   pio run -e esp32dev-sx1262  # Specific env
   ```

4. **Monitor**
   ```bash
   pio device monitor --baud 115200
   ```

### Code Style Guidelines

- **Naming**:
  - `variableName` - camelCase for variables/functions
  - `CONSTANT_NAME` - UPPER_SNAKE_CASE for constants
  - `ClassName` - PascalCase for classes

- **Comments**:
  ```cpp
  // Single line comments for brief explanations
  
  /* Multi-line comments for
     more detailed explanations */
  
  // TODO: Something to do later
  // FIXME: Known bug that needs fixing
  // NOTE: Important clarification
  ```

- **Formatting**:
  - Indentation: 2 spaces or 4 spaces (pick one, be consistent)
  - Braces: K&R style (opening on same line)
  - Line length: Prefer <100 characters

---

## Common Modification Patterns

### Pattern 1: Change Transmission Frequency

```cpp
// In src/main.cpp, find:
#define WU_UPDATE_INTERVAL 15000  // milliseconds

// Change to:
#define WU_UPDATE_INTERVAL 30000  // 30 seconds instead of 15
```

Then rebuild and upload.

### Pattern 2: Add New Service (HTTP Endpoint)

```cpp
// In src/main.cpp, add function:
void sendToMyService() {
    if(!WiFi.isConnected()) return;
    
    HTTPClient http;
    http.begin("https://myservice.com/api/weather");
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<256> doc;
    doc["temp"] = ws.sensor[0].w.temp_c;
    doc["humidity"] = ws.sensor[0].w.humidity;
    
    String json;
    serializeJson(doc, json);
    
    int httpCode = http.POST(json);
    http.end();
}

// Then in loop(), add timer:
static uint32_t lastMyServiceUpdate = 0;
if(millis() - lastMyServiceUpdate > 60000) {  // Every minute
    sendToMyService();
    lastMyServiceUpdate = millis();
}
```

### Pattern 3: Add Custom GPIO for New Device

```cpp
// In src/WeatherSensorCfg.h, add under your board section:
#define PIN_MY_DEVICE 14       // GPIO 14

// In src/main.cpp setup(), initialize:
pinMode(PIN_MY_DEVICE, OUTPUT);

// In loop(), use:
digitalWrite(PIN_MY_DEVICE, HIGH);  // Turn on
delayMicroseconds(100);
digitalWrite(PIN_MY_DEVICE, LOW);   // Turn off
```

### Pattern 4: Add Debug Output

```cpp
// In src/main.cpp:
#if ENABLE_DEBUG_OUTPUT
Serial.printf("[DEBUG] Temperature: %.1f°C\n", temp_c);
#endif
```

### Pattern 5: Modify Unit Conversion

```cpp
// In src/main.cpp, find temperature conversion:
float tempF = (tempC * 9.0 / 5.0) + 32.0;

// To add offset:
float tempF = ((tempC - TEMP_OFFSET) * 9.0 / 5.0) + 32.0;

// Where TEMP_OFFSET = -2.0 for 2°C adjustment
```

---

## Building & Testing

### Build for Specific Environment

```bash
# List available environments
pio run --list-targets

# Build specific environment
pio run -e esp32dev-sx1276

# Build and upload
pio run -e esp32dev-sx1262 -t upload

# Build with verbose output (debug compilation)
pio run -vv
```

### Unit Tests

```bash
# Run existing tests
cd test
make test

# Run specific test
make test TEST=TestRainGauge

# Clean test build
make clean
```

### Memory Analysis

```bash
# Check memory usage after build
pio run
# Output shows RAM and Flash usage

# If memory issues:
# 1. Reduce buffer sizes in src/main.cpp
# 2. Remove unused features with #define guards
# 3. Check for memory leaks (Serial output)
```

---

## Adding Support for New Board

### Step 1: Add to platformio.ini

```ini
[env:my-custom-board]
platform = espressif32
board = esp32dev  # Or specific board ID
framework = arduino
lib_deps =
    ${common_env_data.lib_deps}

build_flags =
    -D ARDUINO_MY_CUSTOM_BOARD
    -D MY_BOARD_VARIANT_SX1262

upload_speed = 921600
monitor_speed = 115200
```

### Step 2: Add GPIO Configuration

In `src/WeatherSensorCfg.h`:

```cpp
#elif defined(ARDUINO_MY_CUSTOM_BOARD)
    #define USE_SX1262
    
    // SPI
    #define PIN_RECEIVER_SCK    18
    #define PIN_RECEIVER_MOSI   23
    #define PIN_RECEIVER_MISO   19
    #define PIN_RECEIVER_CS     27
    
    // IRQ/Control
    #define PIN_RECEIVER_IRQ    21
    #define PIN_RECEIVER_GPIO   33
    #define PIN_RECEIVER_RST    32
    
    // Optional I2C
    #define PIN_SDA     21
    #define PIN_SCL     22
```

### Step 3: Test Build

```bash
pio run -e my-custom-board
```

### Step 4: Update Documentation

- Add to `platformio.ini` environment table
- Update `docs/BOARD_VARIANTS.md`

---

## Contributing Code

### Before Submitting PR

1. **Test Your Changes**
   ```bash
   pio run --target clean
   pio run
   pio run -t upload
   # Verify on serial monitor
   ```

2. **Check Code Style**
   - Follow conventions above
   - Use consistent indentation
   - Add comments for complex logic

3. **Update Documentation**
   - Add changes to `CHANGELOG.md`
   - Update relevant `.md` files
   - Add inline code comments

4. **Test Multiple Environments** (if possible)
   ```bash
   pio run -e esp32dev-sx1262
   pio run -e esp32dev-sx1276
   # Test a few variants
   ```

### Git Workflow

```bash
# Create feature branch
git checkout -b feature/my-new-feature

# Make changes
git add .
git commit -m "Add feature: description"

# Push to your fork
git push origin feature/my-new-feature

# Create Pull Request on GitHub
```

---

## Debugging Techniques

### Serial Monitor Debug Output

```cpp
// Add to serial monitor:
Serial.printf("[SENSOR] Temp: %.1f°C, RH: %u%%\n", 
              ws.sensor[i].w.temp_c, 
              ws.sensor[i].w.humidity);
```

### Memory Debugging

```cpp
// Check free heap memory
uint32_t freeHeap = esp_get_free_heap_size();
Serial.printf("[DEBUG] Free heap: %u bytes\n", freeHeap);

// Alert if low
if(freeHeap < 20000) {
    Serial.println("[WARNING] Low memory!");
}
```

### WiFi Debugging

```cpp
// Check WiFi status
Serial.printf("[WiFi] Connected: %d, Signal: %d dBm\n",
              WiFi.isConnected(),
              WiFi.RSSI());
```

### APRS Debugging

```cpp
// In APRS.cpp:
if(DEBUG) {
    Serial.printf("[APRS] Packet: %s\n", aprs_packet);
}
```

---

## Performance Optimization

### Reduce Memory Usage

1. **Use `const char*` instead of `String`** for constants:
   ```cpp
   // Bad: allocates dynamic memory
   String server = "rotate.aprs.net";
   
   // Good: uses flash memory
   const char* server = "rotate.aprs.net";
   ```

2. **Reduce buffer sizes**:
   ```cpp
   #define RAIN_HISTORY_SIZE 60  // Can reduce if needed
   ```

3. **Remove debug output in production**:
   ```cpp
   #define ENABLE_DEBUG_OUTPUT 0  // Disable serial output
   ```

### Improve Power Consumption

1. **Reduce WiFi transmit power**:
   ```cpp
   WiFi.setTxPower(WIFI_POWER_8dBm);  // Instead of default 20dBm
   ```

2. **Use sleep mode** (advanced):
   ```cpp
   esp_sleep_enable_timer_wakeup(600000000);  // Sleep 10 minutes
   esp_light_sleep_start();
   ```

3. **Disable unnecessary features**:
   ```cpp
   #undef USE_BMP280  // Don't compile BMP280 code
   ```

---

## Testing Strategy

### Unit Tests (in `/test`)

- Test individual components in isolation
- Use mock objects for hardware (Arduino, Preferences, etc.)
- Run without ESP32 hardware required

### Integration Tests

- Test on actual ESP32 hardware
- Verify sensor reception
- Verify WU submission
- Verify APRS connection

### System Tests

- Full end-to-end with real Bresser sensor
- Monitor Weather Underground
- Monitor APRS-IS network (aprs.fi)

---

## Version Bump Process

When releasing a new version:

1. **Update version** in `library.properties`:
   ```
   version=1.0.2
   ```

2. **Update `CHANGELOG.md`**:
   ```markdown
   ## v1.0.2 (2024-01-15)
   ### Added
   - New feature X
   
   ### Fixed
   - Bug Y
   ```

3. **Commit and tag**:
   ```bash
   git add library.properties CHANGELOG.md
   git commit -m "Release v1.0.2"
   git tag v1.0.2
   git push origin main
   git push origin v1.0.2
   ```

4. **Create GitHub Release**:
   - Go to https://github.com/MajakOwO/Bresser2WU/releases
   - Click "Create release"
   - Tag: `v1.0.2`
   - Title: `v1.0.2`
   - Attach binaries from `release_binaries/` folder

---

## Resources

- **ESP32 Arduino Reference**: https://docs.espressif.com/projects/arduino-esp32/
- **PlatformIO Docs**: https://docs.platformio.org
- **RadioLib Documentation**: https://jgromes.github.io/RadioLib/
- **Weather Underground API**: https://www.wunderground.com/weather/api/
- **APRS Protocol**: http://www.aprs.org/doc/

---

## Contact & Support

- **GitHub Issues**: Report bugs
- **GitHub Discussions**: Ask questions
- **Pull Requests**: Submit improvements

---

Happy coding! 🚀
