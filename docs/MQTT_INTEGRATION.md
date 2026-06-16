# MQTT Integration Guide

## Overview

The Bresser2WU firmware supports publishing weather data to an MQTT broker, enabling integration with home automation systems, data logging platforms, and IoT dashboards.

## Configuration

### Web Portal Setup

1. Power on the ESP32 and connect to the `BresserWX` WiFi AP
2. Open browser to `http://192.168.4.1`
3. Scroll to the **MQTT Settings** section on the configuration page
4. Configure the following:

| Setting | Description | Example |
|---------|-------------|---------|
| **MQTT Broker** | Hostname or IP of MQTT server | `192.168.1.100` or `mqtt.example.com` |
| **MQTT Port** | Broker port (default 1883 for unencrypted) | `1883` |
| **MQTT Topic** | Topic to publish weather data to | `weather/station` or `bresserwx/sensor` |
| **MQTT Username** | Username for authentication (optional) | `weather_user` |
| **MQTT Password** | Password for authentication (optional) | `secure_pass` |

### NVS Storage

All MQTT settings are stored in ESP32 NVS (non-volatile storage) under the key `"weather"`:
- `mqttBroker` (string, max 64 chars)
- `mqttPort` (uint16, default 1883)
- `mqttTopic` (string, max 64 chars)
- `mqttUser` (string, max 32 chars, optional)
- `mqttPass` (string, max 32 chars, optional)

## Data Format

### JSON Payload

Weather data is published as a JSON object with the following fields:

```json
{
  "temperature_c": 20.3,
  "temperature_f": 68.5,
  "humidity": 45,
  "dewpoint_c": 7.9,
  "dewpoint_f": 46.3,
  "wind_speed_ms": 1.7,
  "wind_gust_ms": 1.8,
  "wind_speed_mph": 3.8,
  "wind_gust_mph": 4.0,
  "wind_direction": 138,
  "rain_hourly_mm": 0.0,
  "rain_hourly_in": 0.0,
  "rain_daily_mm": 2.5,
  "rain_daily_in": 0.10,
  "precip_rate_mmh": 0.5,
  "precip_rate_inh": 0.02,
  "pressure_hpa": 1013.25,
  "pressure_inhg": 29.92,
  "solar_radiation_wpm2": 450.0
}
```

### Publishing Frequency

- Data is published **every 15 seconds** (synchronized with Weather Underground updates)
- Only publishes when valid sensor data is available (temperature `temp_ok` flag is set)
- Automatically reconnects to broker if connection is lost

## Connection Management

### Automatic Reconnection

The firmware automatically:
1. Connects to the configured MQTT broker on startup
2. Maintains the connection throughout operation
3. Reconnects if the broker becomes unavailable
4. Uses 1883 as default port if not specified

### Buffer Size

The MQTT client buffer is set to **512 bytes** to accommodate the full JSON weather payload, ensuring no data truncation.

## Serial Debug Output

Enable debug output via USB serial (115200 baud) to see MQTT operations:

```
[MQTT] Connecting to 192.168.1.100:1883
[MQTT] Connected
[MQTT] Publish OK: {"temperature_c": 20.3, ...}
```

### Troubleshooting Messages

| Message | Meaning | Solution |
|---------|---------|----------|
| `[MQTT] WiFi nie jest połączone` | WiFi disconnected | Check WiFi connection |
| `[MQTT] Publish FAILED` | Publish failed | Check broker connectivity and credentials |
| `[MQTT] Connect failed, rc=X` | Connection error | See [PubSubClient return codes](https://pubsubclient.knolleary.net/) |

## Home Assistant Integration

### MQTT Discovery (Recommended)

Add the following to your `configuration.yaml`:

```yaml
mqtt:
  broker: 192.168.1.100
  username: weather_user
  password: secure_pass

sensor:
  - platform: mqtt
    name: "Outside Temperature"
    unit_of_measurement: "°C"
    value_template: "{{ value_json.temperature_c }}"
    topic: "weather/station"
    
  - platform: mqtt
    name: "Outside Humidity"
    unit_of_measurement: "%"
    value_template: "{{ value_json.humidity }}"
    topic: "weather/station"
    
  - platform: mqtt
    name: "Wind Speed"
    unit_of_measurement: "m/s"
    value_template: "{{ value_json.wind_speed_ms }}"
    topic: "weather/station"
    
  - platform: mqtt
    name: "Rainfall (24h)"
    unit_of_measurement: "mm"
    value_template: "{{ value_json.rain_daily_mm }}"
    topic: "weather/station"
```

## InfluxDB Integration

For time-series data logging, configure InfluxDB to subscribe to your MQTT topic:

```bash
# Subscribe to MQTT and forward to InfluxDB
mosquitto_sub -h 192.168.1.100 -t "weather/station" | \
  influx write --bucket weather --format=json
```

## Limitations

- Unencrypted connection only (no TLS/SSL support)
- No persistent session - messages in transit during disconnection are lost
- No will/testament message on disconnect
- Maximum payload size: 512 bytes

## Performance Impact

- MQTT operations add ~5ms to each loop cycle
- Memory usage: ~8 KB for MQTT client + 512 bytes buffer
- CPU usage minimal: mostly idle wait for broker response

## Disabling MQTT

If you don't need MQTT functionality:
1. Leave **MQTT Broker** field empty in web portal
2. Firmware will skip MQTT connection and publishing
3. No performance penalty
