# Installation Guide

Complete step-by-step guide to set up Bresser Weather Sensor to WU + APRS-IS Gateway.

## Prerequisites

### Hardware
- **ESP32 Development Board** (choose one):
  - Generic ESP32-DevKit (most affordable)
  - LILYGO TTGO LoRa32 series
  - Heltec Wireless Stick series
  - Any of 20+ supported variants
- **Radio Module** (select one for generic ESP32):
  - SX1262 (recommended - most efficient)
  - SX1276 / RFM95W (widely available)
  - CC1101 (alternative)
  - LR1121 (latest technology)
- **Bresser 7-in-1 Weather Sensor** (or compatible)
- **USB Cable** (for programming and power)
- **Antenna** (for 868 MHz - integrated on some boards)
- **Optional**: BMP280 pressure sensor module

### Software
- **VS Code** with **PlatformIO IDE** extension
- **Python 3.6+** (required by PlatformIO)
- **Git** (for cloning repository)
- **Weather Underground Account** (for data upload)
- **APRS-IS Callsign** (amateur radio, optional but recommended)

### Accounts
- **Weather Underground**: https://www.wunderground.com
  - Create account
  - Add personal weather station
  - Get Station ID and API Key
- **APRS-IS**: https://www.aprs-is.net/
  - Register amateur radio callsign
  - Get APRS passcode (https://www.heywhatsthat.com/aprs_passcode.html)

## Step 1: Hardware Assembly

### Generic ESP32 + SX1262 Radio Module (Recommended)

**Wiring Diagram:**

```
SX1262 Module          ESP32 DevKit
    ┌─────────────────────────────────┐
    │                                 │
  3.3V ────────────────────────────── 3.3V
  GND  ────────────────────────────── GND
    │                                 │
  SPI Bus:                            │
  SCK  (GPIO 5/9) ─────────────────── GPIO 18
  MOSI (GPIO 10)  ─────────────────── GPIO 23
  MISO (GPIO 11)  ─────────────────── GPIO 19
  CS   (GPIO 8)   ─────────────────── GPIO 27
    │                                 │
  Interrupt/Data:                     │
  IRQ  (DIO1) ────────────────────── GPIO 21
  BUSY (GPIO 13)  ─────────────────── GPIO 33
  RST  (GPIO 12)  ─────────────────── GPIO 32
    │                                 │
  Antenna:                            │
  ANTENNA ────── SMA Connector (or direct)
    │                                 │
    └─────────────────────────────────┘

Optional BMP280 Pressure Sensor (I2C):
    BMP280             ESP32
    ┌──────────────────────────┐
  GND ────────────────────── GND
  3.3V ───────────────────── 3.3V
  SDA ────────────────────── GPIO 16
  SCL ────────────────────── GPIO 17
    │                         │
    └──────────────────────────┘
```

### Alternative: Dedicated Board Variant

If using a dedicated board (e.g., **Heltec WiFi LoRa 32 V3**):
- **No wiring needed** - built-in radio module
- Just power via USB-C
- Optional BMP280 can be added via I2C

## Step 2: Software Installation

### Install PlatformIO IDE

1. Open **VS Code**
2. Go to **Extensions** (Ctrl+Shift+X)
3. Search for "**PlatformIO IDE**"
4. Click **Install** (by PlatformIO)
5. After install, click **Reload**
6. You'll see a PlatformIO icon in the sidebar

### Clone Repository

```bash
# Option 1: Using Git
git clone https://github.com/MajakOwO/Bresser2WU.git
cd Bresser2WU
code .

# Option 2: Using ZIP download
# 1. Visit https://github.com/MajakOwO/Bresser2WU
# 2. Click "Code" → "Download ZIP"
# 3. Extract to preferred location
# 4. Open folder in VS Code
```

## Step 3: Configure for Your Hardware

### For Generic ESP32 DevKit

1. Open **`platformio.ini`**
2. Choose your radio module variant:

```ini
# For SX1262 (recommended)
[env:esp32dev-sx1262]

# For SX1276
# [env:esp32dev-sx1276]

# For CC1101
# [env:esp32dev-cc1101]

# For LR1121
# [env:esp32dev-lr1121]
```

3. If using custom GPIO pins, edit **`src/WeatherSensorCfg.h`**:

```cpp
#elif defined(ESP32)
    #define ESP32_VARIANT_SX1262    // Select your module
    
    #if defined(ESP32_VARIANT_SX1262)
        #define USE_SX1262
        #define PIN_RECEIVER_CS   27   // ← Adjust if needed
        #define PIN_RECEIVER_IRQ  21
        #define PIN_RECEIVER_GPIO 33
        #define PIN_RECEIVER_RST  32
```

### For Dedicated Boards

Just select the appropriate environment from `platformio.ini`:

```ini
# Examples
[env:heltec-wifi-lora-32-v3]
[env:ttgo-lora32-v2]
[env:lilygo-t3-s3-sx1262]
```

No manual pin configuration needed - defaults are pre-configured.

## Step 4: Build the Project

### Method 1: Using VS Code UI

1. Click the **PlatformIO** icon (alien 👽) in the sidebar
2. Expand **"esp32dev-sx1262"** (or your chosen environment)
3. Click **Build** button
4. Wait for compilation (2-3 minutes first time)
5. Should see: `✓ [SUCCESS]`

### Method 2: Using Terminal

```bash
# Full build
pio run

# Build specific environment
pio run -e esp32dev-sx1262

# Clean and rebuild
pio run --target clean
pio run
```

### Common Build Errors

**"Board esp32dev not found"**
- Update PlatformIO: `pio system update`

**"RadioLib not found"**
- PlatformIO should auto-install via `lib_deps`
- Force install: `pio lib install "jgromes/RadioLib"`

**"Multiple definitions of..."**
- Use `pio run --target clean` then rebuild

## Step 5: Upload to ESP32

### Step 5a: Connect USB Cable

1. Plug USB cable into ESP32
2. Computer should detect the device
3. On some boards, hold **BOOT** button while plugging in USB

### Step 5b: Identify Serial Port

VS Code should auto-detect, but verify:

```bash
pio device list
```

You should see something like:
```
/dev/ttyUSB0 - USB Serial Port
# or
COM3 - USB Serial Port (Windows)
```

### Step 5c: Upload

**Method 1: VS Code UI**
1. In PlatformIO sidebar, click **Upload** button
2. Wait for upload (30-60 seconds)
3. Should see: `✓ [SUCCESS]`

**Method 2: Terminal**
```bash
# Upload to default environment
pio run -t upload

# Upload to specific environment
pio run -e esp32dev-sx1262 -t upload

# Upload with automatic serial monitor
pio run -t upload && pio device monitor --baud 115200
```

### Common Upload Issues

**"No serial port found"**
- Check USB cable connection
- Try different USB port
- Install USB drivers: https://github.com/espressif/usb_serial_jtag_drivers

**"Device not responding"**
- Hold **BOOT** button
- Click **Upload**
- Release **BOOT** button
- (Or use auto-hold mode if available on your board)

**"Timed out waiting for packet"**
- Try lower upload speed in `platformio.ini`:
```ini
upload_speed = 115200  # Try lower values if fails
```

## Step 6: Monitor Serial Output

After successful upload, the ESP32 will start. Monitor the output:

### Method 1: VS Code UI
1. In PlatformIO sidebar, click **Serial Monitor** button
2. Select COM port (should auto-select)
3. Baud rate: **115200** (auto-selected)

### Method 2: Terminal
```bash
pio device monitor --baud 115200
```

### What You'll See

```
Starting execution...
[INIT] Initializing board...
[INIT] Using on-board transceiver
[INIT] Radio chip: [SX1262]
[INIT] Pin config: RST->32, CS->27, GD0/G0/IRQ->21, GDO2/G1/GPIO->33

[WiFi] Starting WiFiManager...
[WiFi] AutoConnect timeout: 300 seconds

# If WiFi not saved:
[WiFi] Starting AP: BresserWX
[WiFi] Connect to AP at 192.168.4.1
```

## Step 7: Initial WiFiManager Configuration

After first upload, ESP32 will NOT have WiFi credentials.

### First Time Setup

1. On your phone/computer, look for WiFi network: **`BresserWX`**
2. Connect to it (no password needed)
3. Open browser to: **`192.168.4.1`**
4. A configuration page should appear automatically (captive portal)
5. If not, manually visit `192.168.4.1`

### Configuration Form

Fill in the following:

**WiFi Settings:**
- SSID: Your WiFi network name
- Password: Your WiFi password

**Weather Underground:**
- Station ID: (e.g., `KZZXY50`) - get from [wunderground.com](https://wunderground.com)
- Key: API key from your WU account

**APRS-IS (optional):**
- Callsign: (e.g., `SQ9ABC-13`) - your amateur radio call
- Password: APRS passcode (get from https://www.heywhatsthat.com/aprs_passcode.html)
- Latitude: (e.g., `52.2297`)
- Longitude: (e.g., `21.0122`)
- Comment: (optional, e.g., "Weather Station")

### Save and Restart

1. Click **Save**
2. Device will restart (watch serial monitor)
3. Should connect to your WiFi
4. Serial output will show connection status

## Step 8: Verify Operation

### Check Serial Monitor

You should see:

```
[WiFi] Connected to SSID: YourNetwork
[WiFi] IP address: 192.168.1.100
[Sensor] Waiting for signal...
[Sensor] Signal from ID: 0x12345678
[WU] Sending to Weather Underground...
[WU] HTTP Response: 200 (success)
[APRS] Connecting to APRS-IS...
[APRS] Connected
[APRS] Sending weather data...
```

### Verify Weather Underground

1. Go to [wunderground.com](https://wunderground.com)
2. Find your station
3. Check for recent data updates
4. Should see temperature, humidity, wind, rainfall

### Verify APRS-IS

1. Go to [aprs.fi](https://aprs.fi)
2. Search for your callsign (e.g., `SQ9ABC-13`)
3. You should see position and weather data
4. Data updates every 10 minutes

## Step 9: Permanent Installation

### Power Supply Options

1. **USB Power**: Plug into USB adapter + wall socket
2. **Battery**: 
   - 4× AA batteries with voltage regulator (3.3V)
   - Lithium cells (3.7V single cell with regulator)
   - USB power bank (best for outdoor)
3. **Solar**: Solar panel + battery + charge controller

### Housing & Antenna

1. **Weatherproof enclosure**: 3D-printed or plastic box
2. **Antenna placement**:
   - Outdoor location for best reception
   - High position (roof/pole)
   - Vertical orientation (for LoRa)
   - Away from metal structures
3. **Cable routing**:
   - Protect antenna cable from weather
   - Strain relief at connectors
   - Coax cable optional (low power)

### WiFi Placement

For best WiFi range:
- Place ESP32 near router (if possible)
- Avoid metal enclosures (mesh blocks signal)
- Consider WiFi extender for remote locations

## Troubleshooting Installation

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed help with:
- Sensor not detected
- WiFi connection issues
- Data not sending
- APRS problems
- And more...

## Next Steps

1. **Configuration**: See [CONFIGURATION.md](CONFIGURATION.md) for advanced setup
2. **Understanding Architecture**: Read [ARCHITECTURE.md](ARCHITECTURE.md)
3. **Troubleshooting**: Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) if issues arise
4. **Development**: See [DEVELOPMENT.md](DEVELOPMENT.md) if modifying code

## Support

- **GitHub Issues**: https://github.com/MajakOwO/Bresser2WU/issues
- **Discussions**: https://github.com/MajakOwO/Bresser2WU/discussions
- **Documentation**: See other `.md` files in `/docs` folder

---

**Happy installation!** 🎉
