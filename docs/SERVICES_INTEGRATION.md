# External Services Integration

Detailed integration guides for Weather Underground and APRS-IS services.

## Table of Contents

1. [Weather Underground Integration](#weather-underground-integration)
2. [APRS-IS Integration](#aprs-is-integration)
3. [Data Mapping](#data-mapping)
4. [Troubleshooting Services](#troubleshooting-services)

---

## Weather Underground Integration

### Overview

Weather Underground (WU) is a community weather network that collects observations from personal weather stations worldwide. This gateway uploads weather data every 15 seconds to your personal station on WU.

**Official Site**: https://www.wunderground.com/

### Prerequisites

1. **Weather Underground Account**
   - Visit: https://www.wunderground.com
   - Click "Join" or login
   - Verify email

2. **Add Personal Weather Station**
   - Go to: https://www.wunderground.com/member/devices
   - Click "Add Device"
   - Select "Add Personal Weather Station"
   - Choose "Other" for hardware type
   - Enter station name (e.g., "Home Weather Station")

3. **Get Station ID and API Key**
   - Navigate to: https://www.wunderground.com/member/devices
   - Click on your station
   - Station Settings → **API Key** and **Station ID** visible in URL

### Configuration

#### Via WiFiManager Portal

1. Boot ESP32 and connect to "BresserWX" WiFi
2. Open browser to `192.168.4.1`
3. Fill in:
   - **Station ID**: (e.g., `KIZZMY50`)
   - **Key**: Your API key

#### Via Code

Edit `src/main.cpp` or use Preferences API:

```cpp
Preferences prefs;
prefs.begin("BresserWX");
prefs.putString("wuID", "KIZZMY50");
prefs.putString("wuKey", "a1b2c3d4e5f6g7h8");
prefs.end();
```

### Data Upload Details

**Endpoint**: `https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php`

**Protocol**: HTTP GET with query parameters

**Frequency**: Every 15 seconds

**Parameters Sent**:

| Parameter | Value | Example |
|-----------|-------|---------|
| `action` | Literal | `updateraw` |
| `ID` | Station ID | `KIZZMY50` |
| `PASSWORD` | API Key | `a1b2c3d4...` |
| `dateutc` | ISO 8601 | `2024-01-15T14:30:45` |
| `tempf` | Temperature (°F) | `72.5` |
| `humidity` | Humidity (%) | `65` |
| `windspeedmph` | Wind speed (mph) | `5.2` |
| `windgustmph` | Wind gust (mph) | `8.3` |
| `winddir` | Direction (degrees) | `180` |
| `rainin` | Hourly rain (inches) | `0.05` |
| `baromin` | Pressure (inHg) | `29.92` |
| `rtfreq` | Report frequency | `15` |

**Example Request**:
```
GET /weatherstation/updateweatherstation.php?action=updateraw&ID=KIZZMY50&PASSWORD=KEY123&dateutc=2024-01-15T14:30:45&tempf=72.5&humidity=65&windspeedmph=5.2&windgustmph=8.3&winddir=180&rainin=0.05&baromin=29.92
```

### Response Codes

| Code | Meaning | Action |
|------|---------|--------|
| 200 | Success | Data accepted |
| 400 | Bad Request | Check parameters |
| 401 | Unauthorized | Check API Key |
| 403 | Forbidden | Check Station ID |
| 500 | Server Error | Retry next cycle |

### Viewing Your Station

1. Go to: https://www.wunderground.com/weather
2. Search for your Station ID
3. Click on your station
4. View current conditions and history

### Advanced Features

#### Accessing Raw API Data

Get JSON data from your station:

```
https://api.wunderground.com/v1/current?stationId=KIZZMY50&format=json&apiKey=YOUR_API_KEY
```

Replace:
- `KIZZMY50` = your Station ID
- `YOUR_API_KEY` = your API key

#### Station History

View data history:
```
https://www.wunderground.com/history/monthly/KIZZMY50/2024/1
```

Replace:
- `KIZZMY50` = your Station ID
- `2024/1` = year/month

### Common Issues

#### Station Not Receiving Data

1. Verify credentials in WiFiManager portal
2. Check ESP32 WiFi connection (see serial monitor)
3. Verify `[WU]` messages appear every 15 seconds
4. Wait 5-10 minutes for data to appear (caching)

#### Wrong Data Displayed

1. Check parameter values in serial monitor
2. Verify unit conversions (°F not °C)
3. Check WU account for unit settings

#### Missing Data

1. Check WiFi connection status
2. Verify API Key hasn't expired
3. Check request timeout (see CONFIGURATION.md)

---

## APRS-IS Integration

### Overview

APRS (Automatic Position Reporting System) is an amateur radio protocol for real-time reporting of position and weather data. APRS-IS is the internet-based component of APRS.

**Official Sites**:
- Main: http://www.aprs.org/
- Server: https://www.aprs-is.net/
- Mapping: https://aprs.fi/

### Prerequisites

1. **Amateur Radio Callsign**
   - Apply at national amateur radio association
   - Examples: FCC (USA), Ofcom (UK), UKE (Poland)
   - Or use temporary: `NOCALL-13`

2. **APRS Passcode**
   - Not the same as your radio password!
   - Generate at: https://www.heywhatsthat.com/aprs_passcode.html
   - Specific to your callsign

3. **Station Location**
   - Latitude (decimal degrees)
   - Longitude (decimal degrees)
   - Must be reasonably accurate for mapping

### Configuration

#### Via WiFiManager Portal

1. Boot ESP32 and connect to "BresserWX"
2. Open browser to `192.168.4.1`
3. Fill in:
   - **Callsign**: e.g., `SQ9ABC-13`
   - **Password**: 5-digit passcode
   - **Latitude**: e.g., `52.2297`
   - **Longitude**: e.g., `21.0122`
   - **Comment**: (optional) e.g., `Weather Station`

#### Via Code

```cpp
Preferences prefs;
prefs.begin("BresserWX");
prefs.putString("aprsCall", "SQ9ABC-13");
prefs.putString("aprsPass", "12345");
prefs.putString("aprsLat", "52.2297");
prefs.putString("aprsLon", "21.0122");
prefs.putString("aprsComment", "Weather Station");
prefs.end();
```

### Data Upload Details

**Server**: `rotate.aprs.net:14580` (TCP)

**Protocol**: APRS-IS (RFC compliant)

**Frequency**: Every 10 minutes

**Beacon Format** (example):

```
SQ9ABC-13>APRS,qAS,SQ9ZZZ:/142030h5213.78N/02100.74E°234/055/A=00480 T072/65/08/R0.05/P29.92 Bat:OK RSSI:-85dBm
```

### APRS Packet Components

#### Header Section

```
SQ9ABC-13      → Callsign with SSID (your identifier)
APRS           → Destination address
qAS,SQ9ZZZ     → APRS-IS path and receiving station
```

#### Position & Direction

```
/142030h       → UTC time (14:20:30)
5213.78N       → Latitude (52°13.78'N)
/02100.74E     → Longitude (021°00.74'E)  
°234/055/A=00480 → Direction 234° / Wind 55 mph / Altitude 480 ft
```

#### Telemetry

```
T072/65/08/R0.05/P29.92
T072           → Temperature: 72°F
/65            → Humidity: 65%
/08            → Wind gust: 8 mph
/R0.05         → Rainfall: 0.05 inches
/P29.92        → Pressure: 29.92 inHg
```

#### Status

```
Bat:OK                → Battery status
RSSI:-85dBm          → Signal strength
eWX Station v1.0     → Device identifier
```

### APRS Callsign Format

**Standard Format**: `CALL-SSID`

- **CALL**: 3-6 character callsign (e.g., `SQ9ABC`)
- **SSID**: 0-15 (secondary station identifier)

**SSID Meanings** (convention):

| SSID | Meaning |
|------|---------|
| 0 | Primary station |
| 1 | Alternate station |
| 3-4 | Mobile station |
| 7 | Temporary operation |
| 9 | Primary digipeater |
| 10 | WIDEn-N digipeater |
| 11 | Gateway station |
| 12-14 | Alternate digipeater |
| 15 | Miscellaneous |

**Examples**:
- `SQ9ABC` or `SQ9ABC-0` = primary weather station
- `SQ9ABC-13` = alternative/temporary
- `SQ9ABC-15` = miscellaneous

### Viewing Your Beacon

1. Visit: https://aprs.fi
2. Search for your callsign (top-right search box)
3. Your beacon appears with:
   - Position marker on map
   - Current weather data
   - Last update timestamp
   - Signal path information

### Data Mapping

APRS data automatically appears on:
- **aprs.fi**: Primary APRS mapping website
- **APRS.net**: Official APRS mapping
- **ADS-B Exchange**: Optional integration
- **RadioReference**: Amateur radio frequency database

### Advanced Features

#### Position Beacon with Weather

Your position is sent every 10 minutes with current weather telemetry:

```cpp
// In APRS::sendWeatherData()
// Automatically encodes:
- Temperature (°F)
- Humidity (%)
- Wind direction and speed
- Wind gust
- Rainfall (hourly)
- Barometric pressure
```

#### APRS Comment

Custom text appears in your beacon (43 characters max):

```
"Weather Station - QTH: Home"
"Temperature & Wind Monitor"
"ESP32 LoRa Weather Reporter"
```

Edit in WiFiManager configuration.

#### Battery Status

Your APRS beacon includes battery status:
- **Bat:OK** = Battery voltage good
- **Bat:LOW** = Battery low (would need recharge)

---

## Data Mapping

### Temperature Conversion

**Internal**: Celsius
**Weather Underground**: Fahrenheit
**APRS**: Fahrenheit

Conversion used: `°F = (°C × 9/5) + 32`

### Wind Speed Conversion

**Internal**: m/s
**Weather Underground**: mph
**APRS**: mph

Conversion used: `mph = m/s × 2.237`

### Pressure Conversion

**Internal**: hPa
**Weather Underground**: inHg
**APRS**: inHg

Conversion used: `inHg = hPa × 0.02953`

### Rainfall Conversion

**Internal**: mm
**Weather Underground**: inches
**APRS**: inches

Conversion used: `in = mm × 0.03937`

### Latitude/Longitude Format

**Decimal Degrees** (used by this project):
```
Latitude: 52.2297 (positive = North)
Longitude: 21.0122 (positive = East)
```

**APRS Format** (degrees/minutes):
```
Position: 5213.78N / 02100.74E
Conversion: DD + (MM.MM/60) = decimal
```

---

## Troubleshooting Services

### Weather Underground Not Receiving

**Symptom**: Data not appearing on WU station

**Diagnostics**:
```bash
# Check via serial monitor:
pio device monitor --baud 115200

# Look for:
[WU] Sending to Weather Underground...
[WU] HTTP Response: 200

# If 4xx error:
[WU] HTTP Response: 401  # Wrong credentials
[WU] HTTP Response: 403  # Invalid Station ID
```

**Solutions**:
1. Verify Station ID (format: KZZXYYY)
2. Verify API Key is current (regenerate if needed)
3. Ensure WiFi is connected
4. Check endpoint: `rtupdate.wunderground.com` is accessible

### APRS Beacon Not Appearing

**Symptom**: No beacon on aprs.fi for your callsign

**Diagnostics**:
```bash
# Check via serial monitor:
[APRS] Connected
[APRS] Sending weather data...
[APRS] Packet sent

# Or errors:
[APRS] Authentication failed
[APRS] Connection lost
```

**Solutions**:
1. Verify callsign format (CALL-SSID)
2. Verify passcode (regenerate at heywhatsthat.com)
3. Ensure coordinates are valid (decimal degrees)
4. Check WiFi connection
5. Wait 5-10 minutes for map update

### Service Connection Timeouts

**Symptom**: Frequently shows connection lost or timeout

**Causes**:
- WiFi signal too weak
- High packet loss
- Firewall blocking ports

**Solutions**:
1. Move device closer to WiFi router
2. Check router firewall settings:
   - Allow outgoing port 80 (HTTP/WU)
   - Allow outgoing port 14580 (TCP/APRS)
3. Increase timeout values:
   ```cpp
   // In src/main.cpp
   http.setTimeout(20000);  // Increase from 10000
   ```

### Duplicate or Missing Data

**APRS**: Multiple beacons appearing

**Cause**: Using same callsign on multiple devices

**Solution**:
- Use different SSID for each device
- Example: `SQ9ABC-13`, `SQ9ABC-14`, `SQ9ABC-15`

**WU**: Missing data points

**Cause**: WiFi dropout or transmission error

**Solution**:
- Check WiFi stability
- Enable verbose logging for debugging
- Verify no interference nearby

---

## Integration Monitoring

### Command-Line Monitoring

```bash
# Monitor Weather Underground requests
curl -v "https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?action=updateraw&ID=KIZZMY50&PASSWORD=KEY&dateutc=now&tempf=72&humidity=65"

# Monitor APRS server (terminal)
telnet rotate.aprs.net 14580
# Type: user CALLSIGN pass -1
# Quit with: Ctrl+]
```

### Data Validation

Before sending, gateway validates:

**Weather Underground**:
- ✓ Temperature: -20°C to +60°C
- ✓ Humidity: 0-100%
- ✓ Wind: 0-100 mph
- ✓ Rain: non-negative
- ✓ Pressure: 300-1100 hPa

**APRS**:
- ✓ Callsign format valid
- ✓ Passcode numeric
- ✓ Latitude: -90 to +90
- ✓ Longitude: -180 to +180
- ✓ Coordinates not at (0,0)

---

## Performance Tuning

### Reduce Data Transmission Frequency

Edit `src/main.cpp`:

```cpp
// Default: 15 seconds
#define WU_UPDATE_INTERVAL 30000  // 30 seconds

// Default: 10 minutes
#define APRS_UPDATE_INTERVAL 1800000  // 30 minutes
```

### Power Optimization

For battery operation:

```cpp
// Reduce WiFi transmit power
WiFi.setTxPower(WIFI_POWER_8dBm);  // Default: 20dBm

// Increase transmission intervals
#define WU_UPDATE_INTERVAL 60000   // Every minute
#define APRS_UPDATE_INTERVAL 3600000  // Every hour
```

---

## Support & Resources

- **Weather Underground**: https://www.wunderground.com/weather/api/
- **APRS Official**: http://www.aprs.org/
- **APRS-IS**: https://www.aprs-is.net/
- **aprs.fi**: https://aprs.fi/
- **APRS Passcode**: https://www.heywhatsthat.com/aprs_passcode.html
- **APRS Format**: https://www.aprs.org/aprs11/spec-wx.txt

---

For more help, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md) and [FAQ.md](FAQ.md).
