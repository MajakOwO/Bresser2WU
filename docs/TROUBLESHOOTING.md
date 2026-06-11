# Troubleshooting Guide

Common issues and solutions for Bresser Weather Sensor to WU + APRS-IS Gateway.

## Table of Contents

1. [Installation Issues](#installation-issues)
2. [Sensor Reception Problems](#sensor-reception-problems)
3. [WiFi Connection Problems](#wifi-connection-problems)
4. [Weather Underground Issues](#weather-underground-issues)
5. [APRS-IS Connection Problems](#aprs-is-connection-problems)
6. [Data Display Issues](#data-display-issues)
7. [Hardware Issues](#hardware-issues)
8. [Firmware Issues](#firmware-issues)

---

## Installation Issues

### Issue: "Board esp32dev not found" during build

**Error Message**:
```
Error: Unknown board ID 'esp32dev'
```

**Cause**: PlatformIO doesn't have ESP32 platform installed

**Solution**:
```bash
# Update PlatformIO
pio system update

# Update board definitions
pio boards --installed

# If still not found, manually add:
pio platform install espressif32
```

---

### Issue: Upload fails with "Device not responding"

**Error Message**:
```
Timed out waiting for packet header
```

**Causes & Solutions**:

1. **USB Cable Issue**
   - Try different USB cable (data cable, not power-only)
   - Try different USB port on computer
   - Try USB hub instead of direct port

2. **Board Mode**
   - Press and hold **BOOT** button on ESP32
   - Start upload
   - Release **BOOT** when upload begins
   - Some boards auto-enter boot mode (no manual action needed)

3. **USB Driver Missing**
   - **Windows**: Install CP210x driver
     - https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - **Mac**: Usually auto-detected
   - **Linux**: Usually auto-detected

4. **Wrong COM Port**
   - List devices: `pio device list`
   - Check Task Manager (Windows) / System Report (Mac) / dmesg (Linux)
   - Specify in platformio.ini:
     ```ini
     upload_port = COM3  # Windows
     # or
     upload_port = /dev/ttyUSB0  # Linux
     ```

5. **Baud Rate Issue**
   - Try lower speed in platformio.ini:
     ```ini
     upload_speed = 115200  # Default
     # Try slower if fails:
     upload_speed = 921600  # Or lower
     ```

---

### Issue: Build fails with "RadioLib not found"

**Error Message**:
```
fatal error: RadioLib.h: No such file or directory
```

**Cause**: Library dependency not installed

**Solution**:
```bash
# Force library installation
pio lib install "jgromes/RadioLib"

# Or clean and rebuild
pio run --target clean
pio run
```

---

## Sensor Reception Problems

### Issue: "Waiting for signal..." forever in serial monitor

**Serial Output**:
```
[Sensor] Waiting for signal...
[Sensor] Waiting for signal...
[Sensor] Waiting for signal...
```

**Causes & Solutions**:

1. **Sensor Not Transmitting**
   - Check sensor battery (usually CR2032)
   - Replace battery if low
   - Sensor should blink LED every 10-20 seconds when sending

2. **Frequency Mismatch**
   - Verify your location: most use 868 MHz in Europe
   - Some regions use 915 MHz (need different sensor)
   - Check sensor documentation for frequency

3. **Radio Module Not Working**
   - Check antenna is connected
   - Verify antenna is physically plugged into SMA connector
   - Try antenna on different module (if available)
   - Check wiring of radio module to ESP32:
     ```
     SPI: SCK(18), MOSI(23), MISO(19)
     CS(27), IRQ(21), GPIO(33), RST(32)
     ```

4. **Wrong Radio Module Selected**
   - In `platformio.ini`, verify correct environment:
     ```ini
     [env:esp32dev-sx1262]  # ← Check this
     ```
   - Or in `src/WeatherSensorCfg.h`:
     ```cpp
     #define ESP32_VARIANT_SX1262  # Should be enabled
     ```

5. **Low Signal Strength**
   - Move antenna outdoors
   - Raise antenna higher (metal roof preferred)
   - Remove obstacles between sensor and receiver
   - Check RSSI value in serial output:
     ```
     [Sensor] RSSI: -85 dBm (good)
     [Sensor] RSSI: -120 dBm (poor - move antenna)
     ```

6. **Multiple Sensors Interference**
   - Other 868 MHz devices nearby
   - Try different location temporarily
   - Check for WiFi/LoRa gateway in area (rare)

---

### Issue: Sensor data received but values seem wrong

**Example**:
```
Temperature: 45°C (should be ~20°C)
Humidity: 200% (impossible - should be 0-100%)
```

**Causes & Solutions**:

1. **Sensor Malfunction**
   - Restart sensor (remove/reinsert battery)
   - Check for water damage or corrosion
   - Try sensor on different receiver (if available)

2. **Decoding Error**
   - Check serial output for "CRC error" or "checksum failed"
   - Indicates corrupt signal - move antenna or replace sensor

3. **Unit Conversion Issue**
   - Verify in serial monitor what units are being shown
   - Code should use °C internally, convert to °F for WU
   - Check if your Weather Underground station expects °F or °C

---

## WiFi Connection Problems

### Issue: WiFiManager AP "BresserWX" not appearing

**Problem**: Can't find WiFi network to configure

**Causes & Solutions**:

1. **Device Not Powered**
   - Check USB power cable is connected
   - Check LED indicators (should be lit)
   - Verify in serial monitor: should see boot messages

2. **Device Crashed**
   - Look at serial monitor
   - Should see: `[WiFi] Starting WiFiManager...`
   - If no output, device crashed (see Firmware Issues)

3. **AP Disabled in Code**
   - In `src/main.cpp`, check WiFiManager timeout:
     ```cpp
     if(!wm.autoConnect("BresserWX")) {
         // Device enters AP mode if timeout reached
     }
     ```
   - Default: 300 seconds. After this, reboots and tries again

4. **WiFi Country Settings**
   - Some regions may have different WiFi regulations
   - Try reboot with antenna disconnected (test)

---

### Issue: Can access 192.168.4.1 but page won't load

**Problem**: WiFiManager portal appears but freezes or shows error

**Causes & Solutions**:

1. **Firewall/Security Software**
   - Temporarily disable antivirus/firewall
   - Try from different device

2. **Browser Compatibility**
   - Try different browser (Chrome, Firefox, Safari)
   - Clear browser cache (Ctrl+Shift+Delete)

3. **Device Out of Memory**
   - Restart device (power cycle)
   - May need to simplify configuration form

4. **IP Conflict**
   - Try direct IP: `192.168.4.1:80`
   - Or try without `:80` suffix

---

### Issue: Connected to WiFi but "No Internet" warning

**Problem**: Device shows WiFi connected but no data sending

**Causes & Solutions**:

1. **DNS Not Working**
   - Check router DNS settings
   - Try static DNS in platformio.ini (advanced)

2. **Firewall Blocking HTTPS**
   - Weather Underground uses HTTPS (port 443)
   - Check router firewall settings
   - Allow outgoing port 443 for HTTP/HTTPS

3. **WiFi Password Wrong**
   - Try reconfiguring via WiFiManager
   - Clear and re-enter WiFi password
   - Check for CAPS LOCK

4. **Wrong WiFi Network**
   - If multiple networks with same name, try specifying BSSID:
     - Advanced WiFiManager option
     - See `src/main.cpp` setupWiFiManager()

---

## Weather Underground Issues

### Issue: Weather Underground not receiving data

**Problem**: No updates in your WU station

**Causes & Solutions**:

1. **Wrong Station ID or Key**
   - Verify Station ID matches your WU account
   - Format should be: `KZZXYYY` or similar
   - Check if your WU account is active (not expired)

2. **API Key Invalid**
   - Regenerate key in WU dashboard
   - Account → My Settings → Weather API → Generate
   - Copy exactly (no extra spaces)

3. **Data Not Sending (Check Serial)**
   - Look for: `[WU] Sending to Weather Underground...`
   - If missing: WU update interval not reached (15 sec)
   - If appears: Check response: `HTTP 200` = success, other = error

4. **Firewall Blocking**
   - Check outgoing traffic allowed to: `rtupdate.wunderground.com:80`
   - Some networks block external updates
   - Try hotspot from phone temporarily (test)

5. **Weather Underground Server Down**
   - Rare but happens
   - Check: https://status.wunderground.com
   - Usually recovers within minutes

6. **Invalid Data Being Sent**
   - Check serial monitor for sensor values
   - Verify temperature/humidity/wind are reasonable
   - Bad data might be rejected by WU

---

### Issue: Very old data showing on Weather Underground

**Problem**: Station showing data from hours ago

**Causes & Solutions**:

1. **Device Not Updating**
   - Check serial monitor: does `[WU]` message appear every 15 sec?
   - If not: check device is powered and has WiFi
   - Check WiFi connection status

2. **Time Sync Issue**
   - WiFi provides time via NTP
   - If time wrong on device, WU rejects data
   - Device should sync time after WiFi connects

3. **Data Server Cache**
   - WU caches data for display
   - Can take 5-10 minutes to show
   - Check raw API if available (advanced)

---

## APRS-IS Connection Problems

### Issue: APRS beacon not appearing on aprs.fi

**Problem**: Can't find station on APRS-IS network

**Causes & Solutions**:

1. **Callsign Not Registered**
   - APRS uses actual amateur radio callsigns
   - Register on: https://www.aprs-is.net
   - Need valid callsign to participate

2. **Wrong Callsign Format**
   - Should be: `CALL-SSID` (e.g., `SQ9ABC-13`)
   - Ranges: 0-15 for SSID (15 = highest priority)
   - Without SSID, defaults to `-0`

3. **Wrong Passcode**
   - Must generate for YOUR specific callsign
   - Generator: https://www.heywhatsthat.com/aprs_passcode.html
   - Each callsign has unique passcode

4. **Coordinates Not Set**
   - Must provide valid latitude/longitude
   - Use decimal format: `52.2297` (not `52°13'47"`)
   - Negative = South/West
   - Check format in serial output

5. **APRS Server Connection Failed**
   - Check serial for: `[APRS] Connected` message
   - If missing: check firewall allows port 14580 (TCP)
   - Try from different network (test)

6. **Wrong APRS Server**
   - Default: `rotate.aprs.net:14580`
   - Alternative: `noam.aprs2.net:14580` (if rotate fails)
   - Change in `src/APRS.cpp`:
     ```cpp
     const char* APRS_SERVER = "rotate.aprs.net";
     ```

---

### Issue: APRS connection lost after sending

**Serial Output**:
```
[APRS] Connected
[APRS] Sending weather data...
[APRS] Connection lost
```

**Causes & Solutions**:

1. **Server Timeout**
   - Normal if device doesn't receive data for 30+ minutes
   - Device should auto-reconnect on next APRS update (10 min)

2. **Invalid Packet**
   - Check serial for error messages
   - Verify coordinates are valid
   - Check wind/temp/humidity values are reasonable

3. **Network Interruption**
   - WiFi dropped during APRS connection
   - Device should reconnect automatically
   - Check WiFi signal strength

4. **Too Many Connections**
   - Server may reject if too many from same IP
   - Rare unless multiple devices using same callsign
   - Try different SSID number (e.g., `-14` instead of `-13`)

---

## Data Display Issues

### Issue: Serial Monitor Shows Gibberish

**Problem**: Characters appear corrupted/unreadable

**Causes & Solutions**:

1. **Wrong Baud Rate**
   - Should be: **115200**
   - Check bottom-right of VS Code monitor
   - Change to 115200 and reconnect

2. **USB Connection Issue**
   - Try different cable
   - Try different USB port
   - Try different device to verify

3. **Serial Buffer Overflow**
   - Device sending data too fast
   - Should not happen with current code
   - Try disabling debug output:
     ```cpp
     // In src/main.cpp
     #define ENABLE_DEBUG_OUTPUT 0
     ```

---

### Issue: Temperature shows in Celsius instead of Fahrenheit

**Problem**: WU displays wrong temperature scale

**Causes & Solutions**:

1. **Station Settings**
   - Check WU account: Settings → Units → Change to °F
   - WU automatically converts based on your region

2. **Code Sending Wrong**
   - Verify conversion in `src/main.cpp`:
     ```cpp
     float tempF = (tempC * 9.0 / 5.0) + 32.0;  // Should have this
     ```
   - If missing: add conversion

---

### Issue: Rain amount looks wrong

**Problem**: Shows 0 mm when it rained, or shows too much

**Causes & Solutions**:

1. **Sensor Not Detecting Rain**
   - Check sensor rain bucket is clean/unblocked
   - Manually tip bucket (verify click sounds)
   - Check sensor battery

2. **Midnight Rollover**
   - Daily rain resets at midnight UTC
   - Check your local timezone vs UTC
   - For tests: temporarily set:
     ```cpp
     #define RAIN_RESET_HOUR 12  // Noon instead of midnight
     ```

3. **Unit Mismatch**
   - Code uses mm internally
   - Converts to inches for WU
   - Check WU expecting same units

---

## Hardware Issues

### Issue: BMP280 Pressure Sensor Not Detected

**Serial Output**:
```
[BMP280] Not found!
[BMP280] Skipping pressure reading
```

**Causes & Solutions**:

1. **I2C Wiring**
   - Check connections:
     ```
     BMP280 GND ↔ ESP32 GND
     BMP280 3.3V ↔ ESP32 3.3V
     BMP280 SDA ↔ GPIO 16
     BMP280 SCL ↔ GPIO 17

     ```
   - Wiggle connector (poor contact)

2. **Wrong I2C Address**
   - BMP280 has two possible addresses: `0x76` or `0x77`
   - Check board markings
   - In `src/WeatherSensorCfg.h`:
     ```cpp
     #define BMP280_ADDR 0x77  // Try 0x77 if 0x76 fails
     ```

3. **BMP280 Not Enabled**
   - Check enabled in `src/WeatherSensorCfg.h`:
     ```cpp
     #define USE_BMP280  // Should not be commented
     ```

4. **Failed to Initialize**
   - Check power supply voltage (should be 3.3V)
   - Try different I2C address
   - Try different I2C pullup resistors (4.7k standard)

5. **BMP280 Broken**
   - Check for physical damage
   - Try on different ESP32 board (if available)
   - Test with I2C scanner tool

---

### Issue: Antenna Doesn't Receive in House

**Problem**: Works outdoors but not inside

**Causes & Solutions**:

1. **Signal Attenuation**
   - Concrete/metal walls block 868 MHz signals
   - Move as close to window as possible
   - Use outdoor antenna on extension cable

2. **Antenna Quality**
   - Built-in antenna (tiny): very weak
   - External SMA antenna: much better (~3-5 meter improvement)
   - Coax cable: minimal loss if short (<10m)

3. **Antenna Orientation**
   - Should be vertical (upright)
   - Horizontal orientation = worst reception
   - Slanted = OK

---

### Issue: ESP32 Gets Very Hot

**Problem**: Board warm to touch

**Causes & Solutions**:

1. **Normal Operation**
   - Slightly warm is OK (efficient power use)
   - Very hot (>60°C) indicates problem

2. **Short Circuit**
   - Check wiring for crossed wires
   - Disconnect BMP280/radio module (test)
   - Check for solder bridges on board

3. **Power Supply Too High**
   - Should be 3.3V (USB provides this)
   - Check if using 5V power
   - Can damage ESP32

---

## Firmware Issues

### Issue: Device keeps rebooting

**Serial Output**:
```
Starting execution...
[INIT] Initializing board...
...reboot...
Starting execution...
[INIT] Initializing board...
```

**Causes & Solutions**:

1. **Stack Overflow**
   - Too much memory allocated
   - Check for infinite loops in code
   - Look for large arrays being created repeatedly

2. **Brownout Detection**
   - Power supply too weak (USB issue)
   - Try different USB port or power adapter
   - Add capacitor (1000μF) near ESP32 power pins

3. **Watchdog Timeout**
   - Device reboots if main loop blocked >5 sec
   - Check for blocking operations (slow I2C reads, etc.)
   - Comment out slow operations to test

4. **Memory Leak**
   - Using all RAM causes reboot
   - Check serial monitor for patterns
   - May need to reduce buffer sizes

---

### Issue: Flashing fails but no error message

**Problem**: Upload seems stuck or disappears

**Causes & Solutions**:

1. **Slow Flash Memory**
   - Normal for large files
   - Can take 1-2 minutes
   - Don't unplug device

2. **Permissions Issue** (Linux/Mac)
   - Grant permission to serial port:
     ```bash
     sudo usermod -a -G dialout $USER  # Linux
     # Mac: Usually automatic
     ```

3. **Out of Flash Space**
   - Check `pio run` output near end:
     ```
     RAM:   [===   ]  45.1% (used 37328 bytes from 82688 max)
     Flash: [==    ]  28.3% (used 372000 bytes from 1310720 max)
     ```
   - If Flash >90%: remove unnecessary features

---

### Issue: Code compiles but behaves strangely

**Problem**: Random crashes, incomplete data, etc.

**Causes & Solutions**:

1. **Stale Compilation**
   - Clean and rebuild:
     ```bash
     pio run --target clean
     pio run
     ```

2. **Include Path Issues**
   - Check all `.h` files are in correct locations
   - Run: `pio run -vv` (verbose) for include paths

3. **Mixed Compiler Flags**
   - Ensure all environments in `platformio.ini` have correct flags
   - No conflicting defines

---

## Still Can't Find Solution?

### Information to Gather

Before asking for help:

1. **Serial Monitor Output** (full startup sequence):
   ```bash
   # Copy-paste entire output
   pio device monitor --baud 115200
   ```

2. **Board Information**:
   - Which ESP32 variant?
   - Which radio module?
   - Any external sensors?

3. **Error Messages**: Exact text from serial monitor

4. **Reproduction Steps**: What did you do before issue appeared?

### Getting Help

- **GitHub Issues**: https://github.com/MajakOwO/Bresser2WU/issues
- **Discussions**: https://github.com/MajakOwO/Bresser2WU/discussions
- **Email**: Contact project maintainer

### Providing Logs

When reporting issues, include:
1. Full serial monitor output (at least 2 minutes)
2. platformio.ini contents (hide sensitive keys)
3. Your board/radio configuration
4. Any custom modifications to code

---

## Quick Diagnostic Checklist

```
☐ Device powers on (USB light visible)
☐ Serial monitor shows boot messages (115200 baud)
☐ WiFi appears: "BresserWX" network
☐ Can access 192.168.4.1 portal
☐ Configured WiFi credentials
☐ Sensor receiving RF signals ([Sensor] signal text appears)
☐ Weather data transmitted ([WU] success messages)
☐ APRS connected ([APRS] Connected appears)
☐ Data visible on Weather Underground
☐ Data visible on aprs.fi
```

If any item fails → check solutions above for that issue.

---

For more information, see other documentation files or contact support.
