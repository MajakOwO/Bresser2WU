# FAQ - Frequently Asked Questions

Quick answers to the most common questions about Bresser Weather Sensor to WU + APRS-IS Gateway.

## Getting Started

### Q: Which ESP32 board should I buy?

**A:** For beginners, start with a **generic ESP32 DevKit** (~$10-15). Choose one of:
- **ESP32-DevKitC** - most common, widely available
- **ESP32-DevKitM-32** - smaller form factor
- Any board with "ESP32" in name

**Why?** Most affordable, well-supported, many tutorials available.

**Advanced users** might prefer dedicated boards:
- **Heltec WiFi LoRa 32 V3** - built-in SX1262 radio + display
- **LILYGO TTGO LoRa32** - built-in radio
- **Seeed Studio XIAO ESP32S3** - smallest size

---

### Q: Do I need to buy a separate radio module?

**A:** Depends on your board:

| Board Type | Radio Included? | Action |
|-----------|-----------------|--------|
| Generic ESP32 DevKit | ❌ No | Buy SX1262 module (~$10) |
| Heltec / LILYGO | ✅ Yes | Just power it |
| Most development boards | ❌ No | Buy module or choose pre-configured board |

**Best module**: SX1262 (most efficient, newest)

---

### Q: Which radio module is best?

**A:** Ranked by recommendation:

1. **SX1262** (Recommended)
   - Most power efficient
   - Newest technology
   - Best range
   - Easy to find

2. **SX1276 / RFM95W** (Alternative)
   - Widely available
   - Good range
   - Higher power consumption
   - Works well

3. **LR1121** (Newest)
   - Latest technology
   - Excellent performance
   - Harder to find
   - Premium price

4. **CC1101** (Budget Alternative)
   - Cheapest
   - More difficult to work with
   - Lower power
   - OK for indoor use

---

## Installation & Setup

### Q: Do I need to be an amateur radio operator?

**A:** For **Weather Underground**: No! Anyone can use it.

For **APRS-IS**: Recommended but not required technically. You need:
- Amateur radio callsign (or get temporary one)
- APRS passcode (derived from callsign)

**Note**: If you want to use APRS, having a callsign is expected amateur radio etiquette.

---

### Q: Can I use this with 5V power instead of 3.3V?

**A:** ⚠️ **No!** ESP32 is 3.3V only:
- 5V will damage the board
- Always use 3.3V regulator if powering from 5V source
- USB power adapter provides correct 3.3V

---

### Q: How do I choose between WiFi and cellular?

**A:** This project uses WiFi only. For cellular:
- Would need cellular modem (e.g., 4G/LTE HAT)
- Requires major code changes
- Not currently supported
- **Recommendation**: Use WiFi, or add WiFi extender for remote locations

---

## Configuration Questions

### Q: How often does data send to Weather Underground?

**A:** **Every 15 seconds** (hardcoded in `src/main.cpp`).

To change:
```cpp
#define WU_UPDATE_INTERVAL 30000  // Change to 30 seconds
```

---

### Q: How often does data send to APRS-IS?

**A:** **Every 10 minutes** (hardcoded in `src/main.cpp`).

To change:
```cpp
#define APRS_UPDATE_INTERVAL 600000  // Change interval (ms)
```

---

### Q: Can I send data to other services (MQTT, custom API)?

**A:** Not built-in, but **easy to add**:

1. Add HTTP client code to `src/main.cpp`
2. Create new function `sendToMyService()`
3. Call it in main loop with timer

See [DEVELOPMENT.md](DEVELOPMENT.md) "Pattern 2: Add New Service" for example.

---

### Q: How do I change GPIO pins?

**A:** Edit `src/WeatherSensorCfg.h`:

```cpp
#if defined(ESP32_VARIANT_SX1262)
    #define PIN_RECEIVER_CS     27   // ← Change this
    #define PIN_RECEIVER_IRQ    21   // ← Or this
    // ...
#endif
```

**Note**: SPI pins (SCK, MOSI, MISO) are hardware-fixed, only CS/IRQ/RST are flexible.

Then rebuild and upload.

---

## Sensor & Data Questions

### Q: Why does my sensor show wrong temperature/humidity?

**A:** Most common causes:

1. **Wrong units** - Check if showing °C when expecting °F
2. **Sensor battery low** - Replace CR2032 battery
3. **Sensor malfunction** - Try replacing sensor
4. **Weather Underground units** - Check WU account settings

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed help.

---

### Q: Can I use multiple sensors?

**A:** **Yes!** WeatherSensor library supports multiple Bresser sensors. They transmit on same frequency with different IDs.

Code already handles `ws.sensor[0]`, `ws.sensor[1]`, etc.

To use multiple:
1. Configure each sensor with unique ID (via sensor setup)
2. Code automatically collects all
3. Modify `src/main.cpp` to use specific sensor:
   ```cpp
   // Currently uses sensor 0 only
   float temp = ws.sensor[0].w.temp_c;
   ```

---

### Q: What's the range of the wireless connection?

**A:** Depends on:
- **Antenna quality**: External antenna ~3-5x better
- **Obstacles**: Wood OK, metal/concrete bad
- **Radio module**: SX1262 best range (~500m open area)
- **Bresser sensor**: Usually 50-200m in buildings

**Best practice**: Place receiver outdoors or near window.

---

### Q: Does this work in countries outside Europe?

**A:** Mostly, but with limitations:

**868 MHz (Europe)**: ✅ Full support
**915 MHz (North America)**: ⚠️ Partial - need different radio module
**923 MHz (Asia-Pacific)**: ⚠️ Partial

**Note**: Bresser sensor availability varies by region.

---

## Troubleshooting Questions

### Q: Device boots but doesn't receive sensor signals

**A:** Check in order:
1. ✅ Sensor has good battery (try replacing CR2032)
2. ✅ Antenna physically connected to radio module
3. ✅ Antenna raised high and outdoors if possible
4. ✅ Radio module wiring correct (see [INSTALLATION.md](INSTALLATION.md))
5. ✅ Correct radio module selected in `platformio.ini`

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) section "Sensor Reception Problems".

---

### Q: WiFiManager not appearing, can't configure

**A:** Confirm in order:
1. ✅ Device powered (USB light on)
2. ✅ Device booted (check serial monitor)
3. ✅ Looking for WiFi: "BresserWX"
4. ✅ Connected to "BresserWX" network
5. ✅ Opened browser to `192.168.4.1`

If still stuck:
```bash
pio device monitor --baud 115200
```

Check serial output - should show `[WiFi] Starting AP: BresserWX`

---

### Q: Data not appearing on Weather Underground

**A:** Verify:
1. ✅ Station ID from WU (format: `KZZXYYY`)
2. ✅ API Key generated in WU account
3. ✅ WiFi connected (check serial monitor)
4. ✅ Sensor data received (check serial)
5. ✅ `[WU]` messages appearing every 15 seconds

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) section "Weather Underground Issues".

---

### Q: Device keeps rebooting

**A:** Common causes:
1. **Weak power supply** - Use good USB cable + charger
2. **Code crash** - Check serial monitor for errors
3. **Memory leak** - Uncomment `Serial.printf("[DEBUG] Free: %u\n", esp_get_free_heap_size());`

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) section "Device keeps rebooting".

---

## APRS Questions

### Q: Do I need an amateur radio license?

**A:** Technically no for using APRS-IS (internet), but:
- Amateur radio license = your personal callsign
- Without license = not proper etiquette
- Can use "temporary" callsign (format: `NOCALL-13`)
- **Recommendation**: Get licensed if using regularly

---

### Q: How do I get my APRS passcode?

**A:** Visit: https://www.heywhatsthat.com/aprs_passcode.html

1. Enter your callsign (e.g., `SQ9ABC`)
2. Copy the 5-digit code
3. Enter in WiFiManager configuration

**Important**: Each callsign has unique passcode. Don't share!

---

### Q: Where do I see my APRS beacon?

**A:** Go to: https://aprs.fi

1. Search for your callsign (top-right)
2. Your beacon appears with position and weather data
3. Updates every 10 minutes

**Note**: May take a few minutes to first appear.

---

### Q: Why isn't my APRS beacon appearing?

**A:** Check:
1. ✅ Valid amateur radio callsign (or NOCALL-13)
2. ✅ Correct passcode (from heywhatsthat.com)
3. ✅ Valid latitude/longitude (decimal degrees)
4. ✅ WiFi connected
5. ✅ Serial monitor shows `[APRS] Connected`

If still not working, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md) section "APRS-IS Connection Problems".

---

## Advanced Questions

### Q: Can I use BMP280 pressure sensor?

**A:** **Yes!** Optional sensor for barometric pressure.

1. Wire I2C: SCL→GPIO22, SDA→GPIO21
2. Enable in `src/WeatherSensorCfg.h`:
   ```cpp
   #define USE_BMP280
   ```
3. Rebuild and upload

Pressure data automatically included in APRS and WU transmissions.

---

### Q: Can I power this from a battery?

**A:** **Yes**, with considerations:
- ESP32 typical: 80-150 mA WiFi, 40 mA idle
- **USB power bank**: Works great (12-20h with 10000mAh)
- **AA batteries**: Use 4× with 3.3V regulator
- **Solar + battery**: For months of operation

**Tip**: Configure WiFi and APRS update intervals to reduce power.

---

### Q: Can I use this indoors only?

**A:** Works but limited:
- Metal/concrete walls block 868 MHz signals ~30-50% efficiency
- Place receiver near window for best results
- Can still work if sensor close enough (~10-30m through walls)
- Outdoor antenna extension recommended

---

### Q: How do I see debug output?

**A:** Connect serial monitor:
```bash
pio device monitor --baud 115200
```

Shows detailed messages about:
- WiFi connection
- Sensor reception
- Data transmission
- Any errors

---

## Technical Questions

### Q: What's the difference between platformio.ini environments?

**A:** Each environment = different ESP32 board + radio module combination:
- `esp32dev-sx1262` = generic ESP32 + SX1262 module
- `heltec-wifi-lora-32-v3` = Heltec board (built-in SX1262)
- `ttgo-lora32-v2` = LILYGO board (built-in SX1276)

Choose the one matching your hardware.

---

### Q: Can I contribute code improvements?

**A:** **Yes!** Please:
1. Fork repository on GitHub
2. Create feature branch
3. Make improvements
4. Test thoroughly
5. Submit Pull Request

See [DEVELOPMENT.md](DEVELOPMENT.md) for details.

---

### Q: How do I report bugs?

**A:** Create GitHub Issue:
1. https://github.com/MajakOwO/Bresser2WU/issues
2. Describe problem clearly
3. Include serial monitor output
4. Include your board/radio configuration

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) "Getting Help" for info to include.

---

## Performance Questions

### Q: Can I reduce power consumption?

**A:** Several approaches:
1. Reduce WiFi transmit power:
   ```cpp
   WiFi.setTxPower(WIFI_POWER_8dBm);  // Default is 20dBm
   ```

2. Increase transmission intervals:
   ```cpp
   #define WU_UPDATE_INTERVAL 30000   // 30 seconds instead of 15
   #define APRS_UPDATE_INTERVAL 1800000  // 30 minutes instead of 10
   ```

3. Disable debug output:
   ```cpp
   #define ENABLE_DEBUG_OUTPUT 0
   ```

See [DEVELOPMENT.md](DEVELOPMENT.md) "Performance Optimization".

---

### Q: Why is my device getting hot?

**A:** Slightly warm is normal. Very hot indicates:
1. **Short circuit** - Check wiring
2. **Wrong voltage** - Should be 3.3V, not 5V
3. **Broken component** - Replace ESP32

Disconnect and investigate if too hot to touch!

---

## Still Have Questions?

1. **Check Documentation**: [ARCHITECTURE.md](ARCHITECTURE.md), [CONFIGURATION.md](CONFIGURATION.md), etc.
2. **Search Troubleshooting**: [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
3. **Development Guide**: [DEVELOPMENT.md](DEVELOPMENT.md)
4. **Open GitHub Issue**: https://github.com/MajakOwO/Bresser2WU/issues
5. **Start Discussion**: https://github.com/MajakOwO/Bresser2WU/discussions

---

Happy weathering! 🌦️
