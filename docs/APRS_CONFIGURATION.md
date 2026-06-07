# APRS-IS Integration Guide

## Overview

This project now includes support for sending weather station data to APRS-IS (Automatic Packet Reporting System - Internet Service). APRS-IS allows you to share real-time weather data over the internet with the global APRS network.

**NEW: APRS parameters can now be configured via WiFiManager portal without recompiling!**

## What is APRS?

APRS is a real-time tactical information system for rapidly exchanging station data. The APRS-IS component allows amateur radio operators and weather station owners to report their position and sensor data to a worldwide network accessible via the internet.

## Quick Configuration via WiFiManager

### First Run Setup

1. Power on the ESP32
2. Connect to WiFi AP `BresserWX` (if not already connected)
3. Open your browser and go to the WiFiManager portal (usually `192.168.4.1`)
4. Click "Configure WiFi" or let it show the configuration page
5. Scroll down to find the custom APRS parameters:
   - **APRS Callsign**: e.g., `SQ9XXX-13`
   - **APRS Password**: Your APRS-IS password
   - **Latitude**: e.g., `52.1234`
   - **Longitude**: e.g., `21.0567`
6. Fill in the values and save
7. Device will reboot and connect to your WiFi

### Modifying APRS Settings Later

1. Open WiFiManager portal:
   - **Option A**: Press and hold the WiFiManager reset button
   - **Option B**: Connect to `BresserWX` WiFi AP if normal WiFi is unavailable
2. Navigate to configuration page
3. Update APRS parameters
4. Save and reboot

### What Gets Saved

All APRS parameters are automatically saved to the ESP32's internal flash memory (Preferences):
- APRS Callsign
- APRS Password
- Station Latitude
- Station Longitude

Settings persist across reboots and power cycles.

## Manual Configuration (Hardcoded)

## Manual Configuration (Hardcoded)

If you prefer to configure APRS settings directly in code (not recommended):

1. Edit `src/main.cpp` and update the APRS parameters:

```cpp
String aprsCallsign = "YOUR_CALL-13";
String aprsPassword = "YOUR_PASSWORD";
float aprsLatitude = 52.1234;      // Your latitude
float aprsLongitude = 21.0567;     // Your longitude
```

2. Compile and upload to your ESP32

---

### Setting Your APRS Callsign

1. **Obtain a callsign**: You need an amateur radio callsign (e.g., `N0CALL`). This can be obtained from your national amateur radio licensing authority.

2. **Add SSID**: APRS allows multiple data sources from the same callsign using Service Identifiers (SSID). Weather stations typically use `-13` or `-14`:
   - Example: `N0CALL-13` for a weather station

3. **Generate APRS Password**: 
   - For receive-only operation, use any value like `"65435"` (not validated by server)
   - For authenticated transmission, generate your APRS-IS password:
     - Use an APRS password generator online
     - Or calculate using the APRS algorithm (sum of callsign characters)
     - Common online generators: http://www.heywhatsthat.com/aprs_passcode.html

### Setting Your Position

Latitude and Longitude in decimal degrees:
- **Latitude**: Positive for North (N), Negative for South (S)
- **Longitude**: Positive for East (E), Negative for West (W)

Examples:
- New York: `40.7128, -74.0060`
- London: `51.5074, -0.1278`
- Warsaw: `52.2297, 21.0122`

## Weather Data Transmitted

The APRS packet includes:

| Parameter | Format | Example |
|-----------|--------|---------|
| **Position** | DM.MM format | N5200.12/W10700.34 |
| **Wind Direction** | Degrees (0-360) | _225 = southwest |
| **Wind Speed** | mph | /045 avg speed |
| **Wind Gust** | mph | /055 gust speed |
| **Temperature** | °F | t068 = 68°F |
| **Humidity** | 0-99% | h65 = 65% |
| **Pressure** | mb × 10 | P10125 = 1012.5mb |
| **Rainfall (1h)** | 0.01" increments | R00125 = 1.25" |
| **Solar Radiation** | W/m² | (reported to APRS server) |

## APRS-IS Servers

The default server is `rotate.aprs.net:14580` which provides automatic load balancing across regional APRS-IS servers.

Alternative servers:
- `noam.aprs.net:14580` - North America
- `euro.aprs.net:14580` - Europe
- `asia.aprs.net:14580` - Asia

## Viewing Your Data

Once APRS is configured and transmitting, you can view your station's data on:

- **APRS.fi**: http://aprs.fi/?call=YOUR_CALL-13 (most popular)
- **APRSdirect**: http://aprs.direct/
- **OpenAPRS**: http://openaprs.net/

Search for your callsign (including SSID) to see real-time position and weather data.

## Serial Output

Monitor the serial console for APRS connection status:

```
[APRS] Initialized with callsign: YOUR_CALL-13 at 52.2297, 21.0122
[APRS] Connecting to rotate.aprs.net:14580
[APRS] Connected!
[APRS] Sent login: user YOUR_CALL-13 pass *** vers Bresser2APRS 1.0
[APRS] Weather data sent successfully
```

## Disabling APRS

To disable APRS transmission:

**Via WiFiManager:**
1. Open WiFiManager portal
2. Clear the "APRS Callsign" field (leave it empty)
3. Save

**Via Code:**
Set empty string in `main.cpp` (if using hardcoded config):
```cpp
String aprsCallsign = "";  // Empty string disables APRS
```

When `aprsCallsign` is empty, APRS connection attempts are skipped.

## Advanced Configuration

### Changing APRS-IS Server

To use a different APRS-IS server, edit `src/APRS.h`:

```cpp
const char* aprsServer = "euro.aprs.net";  // Change server
const uint16_t aprsPort = 14580;            // Standard port
```

Then recompile and upload.

### Updating via WiFiManager at Runtime

**No recompilation needed!** All APRS parameters are:
- Loaded from WiFiManager portal on boot
- Saved to ESP32's internal flash (Preferences)
- Persisted across power cycles
- Updated whenever you change them in WiFiManager

## Troubleshooting

### Connection Issues

**Problem**: `[APRS] Connection failed` - repeated in serial output

**Solutions**:
1. Check WiFi connection is active: `[APRS] Connection attempt failed`
2. Verify internet connectivity from the device
3. Check firewall/ISP blocking port 14580 (TCP)
4. Try a different APRS-IS server in `APRS.h`
5. Ensure at least 10KB free memory on ESP32

### Login Failures

**Problem**: No response after login attempt or `[APRS] Server: rejected`

**Solutions**:
1. Verify callsign format via WiFiManager (e.g., `SQ9XXX-13`, not `SQ9XXX`)
2. Check password is correct (use `-1` for RX-only mode)
3. Ensure no special characters or spaces in callsign
4. Use http://www.heywhatsthat.com/aprs_passcode.html to verify password

### Data Not Appearing on APRS.fi

**Problem**: Station shows up but weather data doesn't update

**Solutions**:
1. Check serial output shows `[APRS] Sent:` messages regularly
2. Verify all weather sensors are functioning and reading valid values
3. Check latitude/longitude are not 0,0 (use WiFiManager to set correct position)
4. Ensure temperature reading is valid (not showing errors)
5. Wait a few minutes for APRS-IS to propagate data to APRS.fi

### Serial Output Shows Connection Error

**Problem**: `Connection attempt failed` repeated every 5 minutes

**Solutions**:
1. Very likely firewall/port blocking issue
2. Contact your ISP if port 14580 is blocked
3. Try using a mobile hotspot to test
4. Check if APRS-IS server is accessible from a computer on same network

## References

- **APRS.fi**: http://aprs.fi - View real-time APRS data (search by callsign)
- **APRS Specification**: http://www.aprs.org/
- **APRS-IS Documentation**: http://www.aprs-is.net/
- **APRS Passcode Generator**: http://www.heywhatsthat.com/aprs_passcode.html
- **Maidenhead Locator System**: http://en.wikipedia.org/wiki/Maidenhead_Locator_System
- **RadioLib Library**: https://github.com/jgromes/RadioLib

## Notes

- Data is sent to APRS-IS every 15 seconds (same interval as Weather Underground)
- APRS uses the passcode for authentication; incorrect password still allows read-only connection
- The connection automatically reconnects if dropped (5-minute retry interval to avoid flooding)
- No data is stored locally on the device; each packet is independent
- WiFiManager parameters are stored in ESP32 Preferences with 4096-byte limit per key

## Example Configuration for Bresser 7-in-1 Weather Station

**Via WiFiManager Portal:**
1. APRS Callsign: `SQ9ABC-13`
2. APRS Password: `12345`
3. Latitude: `52.2297`
4. Longitude: `21.0122`

After saving, device will:
- Transmit weather data every 15 seconds to APRS-IS
- Save settings to flash memory
- Automatically reconnect if disconnected
- Data will appear at: http://aprs.fi/?call=SQ9ABC-13

**If manually configured in code:**
```cpp
String aprsCallsign = "SQ9ABC-13";
String aprsPassword = "12345";
float aprsLatitude = 52.2297;
float aprsLongitude = 21.0122;
```
