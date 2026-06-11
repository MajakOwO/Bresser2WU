///////////////////////////////////////////////////////////////////////////////////////////////////
// main.cpp - Weather Station to WU + APRS-IS Gateway
//
// Receives weather data from Bresser sensors (868 MHz) and forwards to:
// - Weather Underground (WU): HTTP POST every 15 seconds
// - APRS-IS Network: TCP beacon every 10 minutes
//
// Configuration via WiFiManager: WU Station ID/Key, APRS Callsign/Password/Position
// All settings stored in ESP32 NVS (non-volatile storage)
//
// Repository: https://github.com/MajakOwO/Bresser2WU
//
// Requires: BresserWeatherSensorReceiver library, RadioLib, WiFiManager, Preferences
// Optional: BMP280 pressure sensor (I2C: GPIO 16=SDA, GPIO 17=SCL)
//
// MIT License
///////////////////////////////////////////////////////////////////////////////////////////////////

// Includes
#include <Arduino.h>
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"
#include "InitBoard.h"
#include "APRS.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>

// BMP280 Pressure Sensor Support
// The firmware will always attempt to initialize BMP280 at startup.
// If the sensor is not present, it will be skipped gracefully.
#define USE_BMP280
#ifdef USE_BMP280
#ifdef SENSOR_TYPE_CO2
#undef SENSOR_TYPE_CO2
#endif
#include <Adafruit_BMP280.h>
#include <Wire.h>
Adafruit_BMP280 bmp;
#endif
bool bmpPresent = false;

// Default BMP280 I2C address tries (some modules use 0x77)
static constexpr uint8_t BMP280_I2C_ADDR_1 = 0x76;
static constexpr uint8_t BMP280_I2C_ADDR_2 = 0x77;

// Global Objects
Preferences prefs;
WiFiManager wm;
WeatherSensor ws;
APRS aprs;

// Time Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // UTC+1
const int daylightOffset_sec = 3600;  // DST

// Weather Underground Configuration
String wuID = "";
String wuKey = "";

// APRS-IS Configuration
String aprsCallsign = "";
String aprsPassword = "";
float aprsLatitude = 0.0;
float aprsLongitude = 0.0;
String aprsComment = "";

// Rain Gauge History
struct RainSample {
    float rainMM;
};
RainSample rainHistory[60];
int rainIndex = 0;
bool rainBufferFilled = false;
unsigned long lastRainSample = 0;
float rainAtMidnightMM = 0;
int lastDay = -1;

// Timing Configuration
const unsigned long WU_UPDATE_INTERVAL = 15000;  // 15 seconds
const unsigned long APRS_UPDATE_INTERVAL = 600000;  // 10 minutes
unsigned long lastWU = 0;
unsigned long lastAPRS = APRS_UPDATE_INTERVAL;  // allow immediate APRS send on first loop

// Weather Data Structure
struct WeatherData {
    float tempC;
    float tempF;
    float humidity;
    float dewpointC;
    float dewpointF;
    float windSpeedMph;
    float windGustMph;
    float windSpeedMs;
    float windGustMs;
    int windDirection;
    float pressureHPa;
    float pressureInHg;
    float solarRadiation;
    float rainHourlyMM;
    float rainHourlyIn;
    float rainDailyMM;
    float rainDailyIn;
};
WeatherData weatherData;

// Rain Gauge Management Functions

void updateRainHistory(float currentRain) {
    if (millis() - lastRainSample >= 60000) {
        rainHistory[rainIndex].rainMM = currentRain;
        rainIndex++;

        if (rainIndex >= 60) {
            rainIndex = 0;
            rainBufferFilled = true;
        }
        lastRainSample = millis();
    }
}

void updateRainCounters() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    Serial.printf("Time: %02d:%02d Day:%d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday);

    float currentRain = ws.sensor[0].w.rain_mm;

    // First run
    if (rainAtMidnightMM < 0) {
        rainAtMidnightMM = currentRain;
        lastDay = timeinfo.tm_mday;
        prefs.putFloat("rainMid", rainAtMidnightMM);
        prefs.putInt("day", lastDay);
    }

    // Day change
    if (timeinfo.tm_mday != lastDay) {
        rainAtMidnightMM = currentRain;
        lastDay = timeinfo.tm_mday;
        prefs.putFloat("rainMid", rainAtMidnightMM);
        prefs.putInt("day", lastDay);
        Serial.println("New day - rain counter reset");
    }
}

void printRainDebug(float currentRain) {
    Serial.printf("Rain total=%.1f mm | hour=%.1f mm | day=%.1f mm\n",
                  currentRain, weatherData.rainHourlyMM, weatherData.rainDailyMM);
    Serial.printf("Buffer=%d Index=%d\n", rainBufferFilled, rainIndex);
    
    if (rainBufferFilled) {
        Serial.printf("Oldest sample: %.1f mm\n", rainHistory[rainIndex].rainMM);
    } else if (rainIndex > 0) {
        Serial.printf("First sample: %.1f mm\n", rainHistory[0].rainMM);
    }
}

// Setup and Initialization Functions

void setupPreferences() {
    prefs.begin("weather", false);

    // Load Weather Underground configuration from Preferences
    char wuIDBuf[32] = {0};
    char wuKeyBuf[32] = {0};
    size_t len = prefs.getString("wuID", wuIDBuf, sizeof(wuIDBuf));
    if (len > 0) wuID = String(wuIDBuf);
    
    len = prefs.getString("wuKey", wuKeyBuf, sizeof(wuKeyBuf));
    if (len > 0) wuKey = String(wuKeyBuf);

    // Load APRS configuration from Preferences
    char aprsCallsignBuf[20] = {0};
    char aprsPasswordBuf[20] = {0};
    char aprsLatBuf[20] = {0};
    char aprsLonBuf[20] = {0};
    char aprsCommentBuf[64] = {0};
    
    len = prefs.getString("aprsCall", aprsCallsignBuf, sizeof(aprsCallsignBuf));
    if (len > 0) aprsCallsign = String(aprsCallsignBuf);
    
    len = prefs.getString("aprsPass", aprsPasswordBuf, sizeof(aprsPasswordBuf));
    if (len > 0) aprsPassword = String(aprsPasswordBuf);
    
    len = prefs.getString("aprsLat", aprsLatBuf, sizeof(aprsLatBuf));
    if (len > 0) aprsLatitude = atof(aprsLatBuf);
    
    len = prefs.getString("aprsLon", aprsLonBuf, sizeof(aprsLonBuf));
    if (len > 0) aprsLongitude = atof(aprsLonBuf);

    len = prefs.getString("aprsComment", aprsCommentBuf, sizeof(aprsCommentBuf));
    if (len > 0) aprsComment = String(aprsCommentBuf);

    // Initialize rain gauge from preferences
    rainAtMidnightMM = prefs.getFloat("rainMid", -1);
    lastDay = prefs.getInt("day", -1);
}

void setupWiFiManager() {
    char aprsLatBuf[20] = {0};
    char aprsLonBuf[20] = {0};
    snprintf(aprsLatBuf, sizeof(aprsLatBuf), "%.4f", aprsLatitude);
    snprintf(aprsLonBuf, sizeof(aprsLonBuf), "%.4f", aprsLongitude);

    // Setup WiFiManager custom parameters for Weather Underground
    WiFiManagerParameter custom_wu_id("wuID", "Weather Underground Station ID", 
                                      wuID.c_str(), 32);
    WiFiManagerParameter custom_wu_key("wuKey", "Weather Underground Key", 
                                       wuKey.c_str(), 32);

    // Setup WiFiManager custom parameters for APRS
    WiFiManagerParameter custom_aprs_call("aprsCall", "APRS Callsign (e.g., SQ9XXX-13)", 
                                         aprsCallsign.c_str(), 20);
    WiFiManagerParameter custom_aprs_pass("aprsPass", "APRS Password", 
                                         aprsPassword.c_str(), 20);
    WiFiManagerParameter custom_aprs_lat("aprsLat", "Latitude (e.g., 52.1234)", 
                                        aprsLatBuf, 20);
    WiFiManagerParameter custom_aprs_lon("aprsLon", "Longitude (e.g., 21.0567)", 
                                        aprsLonBuf, 20);
    WiFiManagerParameter custom_aprs_comment("aprsComment", "APRS Comment (optional)", 
                                            aprsComment.c_str(), 64);

    wm.addParameter(&custom_wu_id);
    wm.addParameter(&custom_wu_key);
    wm.addParameter(&custom_aprs_call);
    wm.addParameter(&custom_aprs_pass);
    wm.addParameter(&custom_aprs_lat);
    wm.addParameter(&custom_aprs_lon);
    wm.addParameter(&custom_aprs_comment);

    wm.setConfigPortalTimeout(300);

    if (!wm.autoConnect("BresserWX")) {
        ESP.restart();
    }

    // After WiFi connection, read updated WU parameters from WiFiManager
    wuID = String(custom_wu_id.getValue());
    wuKey = String(custom_wu_key.getValue());

    // After WiFi connection, read updated APRS parameters from WiFiManager
    aprsCallsign = String(custom_aprs_call.getValue());
    aprsPassword = String(custom_aprs_pass.getValue());
    aprsLatitude = atof(custom_aprs_lat.getValue());
    aprsLongitude = atof(custom_aprs_lon.getValue());
    aprsComment = String(custom_aprs_comment.getValue());

    // Save configuration to Preferences
    if (wuID.length() > 0) {
        prefs.putString("wuID", wuID);
        prefs.putString("wuKey", wuKey);
        Serial.printf("[WU] Configuration saved: %s\n", wuID.c_str());
    }

    if (aprsCallsign.length() > 0) {
        prefs.putString("aprsCall", aprsCallsign);
        prefs.putString("aprsPass", aprsPassword);
        prefs.putString("aprsLat", String(aprsLatitude, 4));
        prefs.putString("aprsLon", String(aprsLongitude, 4));
        prefs.putString("aprsComment", aprsComment);
        Serial.printf("[APRS] Configuration saved: %s\n", aprsCallsign.c_str());
    }
}

void setupSensors() {
    Serial.println("[SENS] setupSensors: before ws.begin");
    int rc = ws.begin();
    Serial.printf("[SENS] setupSensors: after ws.begin rc=%d\n", rc);
    if (rc != RADIOLIB_ERR_NONE) {
        Serial.printf("[SENS] Radio init failed: %d\n", rc);
        Serial.println("[SENS] Radio receiver disabled");
    }

#ifdef USE_BMP280
    Serial.println("[SENS] setupSensors: before BMP280 init");
    // Your BMP280 wiring (user reports): SDA=16, SCL=17
    Wire.begin(16, 17);
    bmpPresent = bmp.begin(BMP280_I2C_ADDR_1);
    if (!bmpPresent) {
        bmpPresent = bmp.begin(BMP280_I2C_ADDR_2);
    }
    Serial.printf("[SENS] BMP280 init result: %d\n", bmpPresent ? 1 : 0);
#endif

    Serial.println("[SENS] setupSensors: before configTime");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("[SENS] setupSensors: after configTime");

    Serial.println("[SENS] setupSensors: before rain history init");
    for (int i = 0; i < 60; i++) {
        rainHistory[i].rainMM = 0;
    }
    Serial.println("[SENS] setupSensors: after rain history init");
    Serial.println("[SENS] setupSensors: return");
}

void setupAPRS() {
    if (aprsCallsign.length() > 0) {
        aprs.begin(aprsCallsign, aprsPassword, aprsLatitude, aprsLongitude);
        if (aprsComment.length() > 0) {
            aprs.setComment(aprsComment);
        }
        Serial.printf("[APRS] Initialized: %s at %.4f, %.4f\n", 
                     aprsCallsign.c_str(), aprsLatitude, aprsLongitude);
    } else {
        Serial.println("[APRS] Callsign not configured - APRS disabled");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.printf("Starting execution...\n");

    initBoard();
    setupPreferences();
    setupWiFiManager();
    Serial.println("WiFi connected");
    Serial.println("Starting sensor initialization...");
    setupSensors();
    Serial.println("Sensor initialization completed");
    Serial.println("[SETUP] before setupAPRS");
    Serial.println("Starting APRS initialization...");
    setupAPRS();
    Serial.println("APRS initialization completed");

    // Send APRS status on boot (include repo link)
    {
        struct tm timeinfo;
        String status = "System booted";
        if (getLocalTime(&timeinfo)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "System booted at %02d:%02d:%02dZ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            status = String(buf);
        }
        aprs.sendStatus(status);
        // Send repository link as separate status to appear on next line in APRS viewers
        aprs.sendStatus("https://github.com/MajakOwO/Bresser2WU");
    }
}

// Utility Functions

/// Calculate dew point using Magnus formula
float calculateDewPoint(float tempC, float humidity) {
    const float a = 17.27;
    const float b = 237.7;
    float alpha = ((a * tempC) / (b + tempC)) + log(humidity / 100.0);
    return (b * alpha) / (a - alpha);
}

/// Read pressure from BMP280 sensor (if available)
float readPressure() {
#ifdef USE_BMP280
    if (bmpPresent) {
        float pressure = bmp.readPressure() / 100.0;  // Convert to hPa
        if (pressure < 100) {
            Serial.println("BMP280 invalid reading");
            return 0;
        }
        return pressure;
    }
#endif
    return 0;
}

/// Convert weather sensor data to common units
void calculateWeatherData(const WeatherSensor::Sensor& sensor) {
    weatherData.tempC = sensor.w.temp_c;
    weatherData.tempF = sensor.w.temp_c * 9.0 / 5.0 + 32.0;
    weatherData.humidity = sensor.w.humidity;
    weatherData.dewpointC = calculateDewPoint(sensor.w.temp_c, sensor.w.humidity);
    weatherData.dewpointF = weatherData.dewpointC * 9.0 / 5.0 + 32.0;
    weatherData.windSpeedMph = sensor.w.wind_avg_meter_sec * 2.23694;
    weatherData.windGustMph = sensor.w.wind_gust_meter_sec * 2.23694;
    weatherData.windSpeedMs = sensor.w.wind_avg_meter_sec;
    weatherData.windGustMs = sensor.w.wind_gust_meter_sec;
    weatherData.windDirection = (int)sensor.w.wind_direction_deg;
    weatherData.solarRadiation = sensor.w.light_klx * 7.9;
    
    // Pressure
    weatherData.pressureHPa = readPressure();
    weatherData.pressureInHg = weatherData.pressureHPa * 0.0295299831;
    
    // Rain
    weatherData.rainHourlyMM = 0;
    if (rainBufferFilled) {
        weatherData.rainHourlyMM = sensor.w.rain_mm - rainHistory[rainIndex].rainMM;
    } else if (rainIndex > 0) {
        weatherData.rainHourlyMM = sensor.w.rain_mm - rainHistory[0].rainMM;
    }
    if (weatherData.rainHourlyMM < 0) weatherData.rainHourlyMM = 0;
    weatherData.rainHourlyIn = weatherData.rainHourlyMM / 25.4;
    
    weatherData.rainDailyMM = sensor.w.rain_mm - rainAtMidnightMM;
    if (weatherData.rainDailyMM < 0) weatherData.rainDailyMM = 0;
    weatherData.rainDailyIn = weatherData.rainDailyMM / 25.4;
}

// Service Transmission Functions

void sendToWU() {
    if (wuID.length() == 0 || wuKey.length() == 0) {
        return;  // WU not configured
    }

    if (WiFi.status() != WL_CONNECTED) return;

    int const i = 0;
    if (!ws.sensor[i].w.temp_ok) return;

    // Update rain counters and history
    updateRainCounters();
    float currentRain = ws.sensor[i].w.rain_mm;
    updateRainHistory(currentRain);

    // Check if rain counter was reset
    if (currentRain < rainAtMidnightMM) {
        rainAtMidnightMM = currentRain;
        prefs.putFloat("rainMid", rainAtMidnightMM);
    }

    // Calculate all weather data
    calculateWeatherData(ws.sensor[i]);
    
    printRainDebug(currentRain);
    Serial.printf("Dew Point: %.1f C (%.1f F)\n", weatherData.dewpointC, weatherData.dewpointF);
    Serial.printf("BMP280: %.1f hPa (%.2f inHg)\n", weatherData.pressureHPa, weatherData.pressureInHg);

    // Build Weather Underground URL
    String url = "https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?"
        "ID=" + wuID +
        "&PASSWORD=" + wuKey +
        "&dateutc=now" +
        "&tempf=" + String(weatherData.tempF, 1) +
        "&dewptf=" + String(weatherData.dewpointF, 1) +
        "&humidity=" + String(weatherData.humidity) +
        "&windspeedmph=" + String(weatherData.windSpeedMph, 1) +
        "&windgustmph=" + String(weatherData.windGustMph, 1) +
        "&winddir=" + String(weatherData.windDirection) +
        "&rainin=" + String(weatherData.rainHourlyIn, 3) +
        "&dailyrainin=" + String(weatherData.rainDailyIn, 3) +
        "&UV=" + String(ws.sensor[i].w.uv, 1) +

        "&solarradiation=" + String(weatherData.solarRadiation, 1);

#ifdef USE_BMP280
    if (bmpPresent && weatherData.pressureHPa > 0) {
        url += "&baromin=" + String(weatherData.pressureInHg, 2);
    }
#endif

    url += "&softwaretype=ESP32-Bresser7in1&action=updateraw";

    Serial.println(url);
    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);
    int code = http.GET();
    Serial.printf("WU HTTP: %d\n", code);

    if (code > 0) {
        String resp = http.getString();
        Serial.println(resp);
    }
    http.end();
}

bool sendToAPRS() {
    if (aprsCallsign.length() == 0) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    int const i = 0;
    if (!ws.sensor[i].w.temp_ok) return false;

    // Keep rain history updated even when only APRS is enabled
    updateRainCounters();
    float currentRain = ws.sensor[i].w.rain_mm;
    updateRainHistory(currentRain);
    if (currentRain < rainAtMidnightMM) {
        rainAtMidnightMM = currentRain;
        prefs.putFloat("rainMid", rainAtMidnightMM);
    }

    // Connect if needed
    if (!aprs.isConnected()) {
        if (!aprs.connectToServer()) {
            Serial.println("[APRS] Connection attempt failed");
            return false;
        }
    }

    // Calculate weather data (same as WU)
    calculateWeatherData(ws.sensor[i]);

    // Send to APRS with all calculated data
    bool success = aprs.sendWeatherData(
        weatherData.tempF,
        weatherData.humidity,
        weatherData.windSpeedMph,
        weatherData.windGustMph,
        weatherData.windDirection,
        weatherData.rainHourlyMM,
        weatherData.rainDailyMM,
        weatherData.pressureInHg,
        weatherData.solarRadiation,
        ws.sensor[i].battery_ok,
        ws.sensor[i].rssi
    );

    Serial.printf("[APRS] %s\n", success ? "Weather data sent" : "Failed to send weather data");
    return success;
}

// Sensor Data Display Function

void printSensorInfo(const WeatherSensor::Sensor& sensor) {
    char batt_ok[] = " [OK] ";
    char batt_low[] = " [Low] ";
    char batt_inv[] = " [---] ";
    char* batt;

    if ((sensor.s_type == SENSOR_TYPE_WEATHER1) && !sensor.w.temp_ok) {
        batt = batt_inv;
    } else if (sensor.battery_ok) {
        batt = batt_ok;
    } else {
        batt = batt_low;
    }

    Serial.printf("Id: [%8X] Typ: [%X] Ch: [%d] St: [%d] Bat: [%-3s] RSSI: [%6.1fdBm] ",
        static_cast<int>(sensor.sensor_id), sensor.s_type, sensor.chan,
        sensor.startup, batt, sensor.rssi);

    // Print sensor-specific data
    if (sensor.s_type == SENSOR_TYPE_LIGHTNING) {
        Serial.printf("Lightning: [%4d] Dist: [%2dkm]\n", sensor.lgt.strike_count, sensor.lgt.distance_km);
    } else if (sensor.s_type == SENSOR_TYPE_LEAKAGE) {
        Serial.printf("Leakage: [%-5s]\n", sensor.leak.alarm ? "ALARM" : "OK");
    } else if (sensor.s_type == SENSOR_TYPE_AIR_PM) {
        Serial.printf("PM1.0: [%u] PM2.5: [%u] PM10: [%u] µg/m³\n",
            sensor.pm.pm_1_0, sensor.pm.pm_2_5, sensor.pm.pm_10);
    } else if (sensor.s_type == SENSOR_TYPE_CO2) {
        Serial.printf("CO2: [%u ppm]\n", sensor.co2.co2_ppm);
    } else if (sensor.s_type == SENSOR_TYPE_HCHO_VOC) {
        Serial.printf("HCHO: [%u ppb] VOC: [%u]\n", sensor.voc.hcho_ppb, sensor.voc.voc_level);
    } else if (sensor.s_type == SENSOR_TYPE_SOIL) {
        Serial.printf("Temp: [%5.1fC] Moisture: [%2d%%]\n", sensor.soil.temp_c, sensor.soil.moisture);
    } else {
        // Weather-like sensor
        if (sensor.w.temp_ok) {
            Serial.printf("Temp: [%5.1fC] ", sensor.w.temp_c);
        } else {
            Serial.printf("Temp: [---.-C] ");
        }
        if (sensor.w.humidity_ok) {
            Serial.printf("Hum: [%3d%%] ", sensor.w.humidity);
        } else {
            Serial.printf("Hum: [---%%] ");
        }
        if (sensor.w.wind_ok) {
            Serial.printf("Wind: [%4.1f/%4.1f m/s] Dir: [%5.1fdeg] ",
                sensor.w.wind_gust_meter_sec, sensor.w.wind_avg_meter_sec,
                sensor.w.wind_direction_deg);
        } else {
            Serial.printf("Wind: [--.-/--.-m/s] Dir: [---.-deg] ");
        }
        if (sensor.w.rain_ok) {
            Serial.printf("Rain: [%7.1f mm] ", sensor.w.rain_mm);
        } else {
            Serial.printf("Rain: [-----.-mm] ");
        }
#if defined BRESSER_6_IN_1 || defined BRESSER_7_IN_1
        if (sensor.w.uv_ok) {
            Serial.printf("UV: [%2.1f] ", sensor.w.uv);
        }
#endif
#ifdef BRESSER_7_IN_1
        if (sensor.w.light_ok) {
            Serial.printf("Light: [%2.1f klx] ", sensor.w.light_klx);
        }
        if (sensor.s_type == SENSOR_TYPE_WEATHER8 && sensor.w.tglobe_ok) {
            Serial.printf("T_globe: [%3.1fC] ", sensor.w.tglobe_c);
        }
#endif
        Serial.printf("\n");
    }
}

// Main Application Loop

void loop() {
    int const i = 0;

    ws.clearSlots();
    int decode_status = ws.getMessage();

    bool haveSensorData = ws.sensor[i].w.temp_ok;

    if (decode_status == DECODE_OK) {
        printSensorInfo(ws.sensor[i]);
    }

    if (haveSensorData) {
        if (millis() - lastWU > WU_UPDATE_INTERVAL) {
            sendToWU();
            lastWU = millis();
        }

        if (millis() - lastAPRS > APRS_UPDATE_INTERVAL) {
            if (sendToAPRS()) {
                lastAPRS = millis();
            }
        }
    }

    delay(100);
}
