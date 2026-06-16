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
#include <Preferences.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <Update.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>

#define USE_SSD1306

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

#ifdef USE_SSD1306
#include <Adafruit_SSD1306.h>
static const uint8_t SCREEN_WIDTH = 128;
static const uint8_t SCREEN_HEIGHT = 64;
static const uint8_t OLED_RESET = -1;
static const uint8_t OLED_ADDRESS = 0x3C;

#ifdef OLED_SDA
TwoWire displayWire = TwoWire(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &displayWire, OLED_RESET);
#else
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif
#endif

bool bmpPresent = false;

// Default BMP280 I2C address tries (some modules use 0x77)
static constexpr uint8_t BMP280_I2C_ADDR_1 = 0x76;
static constexpr uint8_t BMP280_I2C_ADDR_2 = 0x77;
static const uint8_t I2C_SDA_PIN = 16;
static const uint8_t I2C_SCL_PIN = 17;

// Global Objects
Preferences prefs;
WeatherSensor ws;
APRS aprs;

// OTA Update Server
WebServer otaServer(8080);
WebServer configServer(80);
HTTPUpdateServer otaUpdater;
WiFiClient mqttWiFiClient;
PubSubClient mqttClient(mqttWiFiClient);

String wifiSSID = "";
String wifiPassword = "";
String mqttBroker = "";
uint16_t mqttPort = 1883;
String mqttTopic = "bresserwx/weather";
String mqttUser = "";
String mqttPass = "";

bool weatherDataValid = false;
bool displayPresent = false;
bool wifiConfigured = false;
bool wifiConnected = false;
bool loraPresent = false;
bool loraInitFailed = false;

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

    // Rain totals
    float rainHourlyMM;
    float rainHourlyIn;
    float rainDailyMM;
    float rainDailyIn;

    // Precip rate (intensity)
    float precipRateMMH;  // mm/h
    float precipRateInH;  // inch/h
};
WeatherData weatherData;

// Precip rate baseline (delta of cumulative rain counter over time)
unsigned long lastRainRateTsMillis = 0;
float lastRainRateRainMM = -1;
const unsigned long PRECIP_RATE_UPDATE_INTERVAL = 60000;  // Update precipitation rate every 60 seconds



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

    char wifiSSIDBuf[32] = {0};
    char wifiPassBuf[64] = {0};
    len = prefs.getString("ssid", wifiSSIDBuf, sizeof(wifiSSIDBuf));
    if (len > 0) wifiSSID = String(wifiSSIDBuf);
    len = prefs.getString("pass", wifiPassBuf, sizeof(wifiPassBuf));
    if (len > 0) wifiPassword = String(wifiPassBuf);
    wifiConfigured = wifiSSID.length() > 0;
    wifiConnected = false;

    char mqttBrokerBuf[64] = {0};
    char mqttTopicBuf[64] = {0};
    char mqttUserBuf[32] = {0};
    char mqttPassBuf[32] = {0};
    len = prefs.getString("mqttBroker", mqttBrokerBuf, sizeof(mqttBrokerBuf));
    if (len > 0) mqttBroker = String(mqttBrokerBuf);
    mqttPort = prefs.getInt("mqttPort", 1883);
    len = prefs.getString("mqttTopic", mqttTopicBuf, sizeof(mqttTopicBuf));
    if (len > 0) mqttTopic = String(mqttTopicBuf);
    len = prefs.getString("mqttUser", mqttUserBuf, sizeof(mqttUserBuf));
    if (len > 0) mqttUser = String(mqttUserBuf);
    len = prefs.getString("mqttPass", mqttPassBuf, sizeof(mqttPassBuf));
    if (len > 0) mqttPass = String(mqttPassBuf);

    // Initialize rain gauge from preferences
    rainAtMidnightMM = prefs.getFloat("rainMid", -1);
    lastDay = prefs.getInt("day", -1);
}

bool connectSavedWiFi() {
    if (wifiSSID.length() == 0) {
        Serial.println("[WIFI] No saved WiFi credentials");
        wifiConfigured = false;
        wifiConnected = false;
        return false;
    }
    Serial.printf("[WIFI] Connecting to saved SSID: %s\n", wifiSSID.c_str());
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    wifiConfigured = true;
    wifiConnected = false;

    unsigned long start = millis();
    while (millis() - start < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.printf("[WIFI] Connected to %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
            return true;
        }
        delay(200);
    }
    Serial.println("[WIFI] Saved WiFi connection failed");
    wifiConnected = false;
    return false;
}

void startConfigAP() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("BresserWX");
    Serial.printf("[WIFI] AP started: %s IP: %s\n", "BresserWX", WiFi.softAPIP().toString().c_str());
}

String buildConfigPage(const String& message = "") {
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0,0,0,0)) {
        ip = WiFi.softAPIP();
    }
    String page;
    page.reserve(9000);
    page += "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    page += "<title>BresserWX Configuration</title>";
    page += "<style>body{font-family:Arial,sans-serif;background:#f3f7fb;color:#222;margin:0;padding:0} .container{max-width:860px;margin:0 auto;padding:20px;} .card{background:#fff;border-radius:14px;box-shadow:0 10px 24px rgba(0,0,0,.08);padding:24px;margin-bottom:18px;} h1{margin:0 0 12px;font-size:2rem;color:#0c4b9b;} h2{margin:0 0 12px;font-size:1.3rem;color:#07376b;} label{display:block;margin:12px 0 6px;font-weight:600;} input, textarea{width:100%;padding:10px 12px;border:1px solid #cbd5e1;border-radius:10px;font-size:1rem;} .small{font-size:0.95rem;color:#556677;} .button{display:inline-block;padding:12px 20px;background:#0069d9;color:#fff;border-radius:10px;text-decoration:none;border:none;font-size:1rem;cursor:pointer;} .button-secondary{background:#17a2b8;} .status{padding:12px 14px;border-radius:10px;background:#e9f7f5;color:#0b6b4f;border:1px solid #9ae1c9;margin-bottom:18px;} .field-row{display:grid;grid-template-columns:1fr 1fr;gap:14px;} .field-row.full{grid-template-columns:1fr;} .note{margin-top:6px;font-size:0.9rem;color:#667788;} </style></head><body><div class='container'>";
    page += "<div style='text-align:center;margin-bottom:18px;'><h1>BresserWX Configuration</h1><div class='small'>IP: " + ip.toString() + "</div></div>";
    if (message.length()) {
        page += "<div class='status'>" + message + "</div>";
    }
    page += "<form method='POST' action='/save'>";
    page += "<div class='card'><h2>WiFi Settings</h2>";
    page += "<label for='ssid'>SSID</label><input id='ssid' name='ssid' value='" + wifiSSID + "' placeholder='WiFi network name'>";
    page += "<label for='pass'>Password</label><input id='pass' name='pass' type='password' value='" + wifiPassword + "' placeholder='WiFi password'>";
    page += "<div class='note'>Save network settings so the device can connect and remain available in AP mode.</div></div>";
    page += "<div class='card'><h2>Weather Upload / APRS</h2>";
    page += "<div class='field-row'><div><label for='wuID'>WU Station ID</label><input id='wuID' name='wuID' value='" + wuID + "'></div>";
    page += "<div><label for='wuKey'>WU Key</label><input id='wuKey' name='wuKey' type='password' value='" + wuKey + "'></div></div>";
    page += "<div class='field-row'><div><label for='aprsCall'>APRS Callsign</label><input id='aprsCall' name='aprsCall' value='" + aprsCallsign + "'></div>";
    page += "<div><label for='aprsPass'>APRS Password</label><input id='aprsPass' name='aprsPass' type='password' value='" + aprsPassword + "'></div></div>";
    page += "<div class='field-row'><div><label for='aprsLat'>Latitude</label><input id='aprsLat' name='aprsLat' value='" + String(aprsLatitude, 4) + "'></div>";
    page += "<div><label for='aprsLon'>Longitude</label><input id='aprsLon' name='aprsLon' value='" + String(aprsLongitude, 4) + "'></div></div>";
    page += "<label for='aprsComment'>APRS Comment</label><textarea id='aprsComment' name='aprsComment' rows='3'>" + aprsComment + "</textarea></div>";
    page += "<div class='card'><h2>MQTT Settings</h2>";
    page += "<label for='mqttBroker'>Broker</label><input id='mqttBroker' name='mqttBroker' value='" + mqttBroker + "' placeholder='mqtt.example.com'>";
    page += "<div class='field-row'><div><label for='mqttPort'>Port</label><input id='mqttPort' name='mqttPort' value='" + String(mqttPort) + "' placeholder='1883'></div>";
    page += "<div><label for='mqttTopic'>Topic</label><input id='mqttTopic' name='mqttTopic' value='" + mqttTopic + "' placeholder='bresserwx/weather'></div></div>";
    page += "<div class='field-row'><div><label for='mqttUser'>Username</label><input id='mqttUser' name='mqttUser' value='" + mqttUser + "' placeholder='optional'></div>";
    page += "<div><label for='mqttPass'>Password</label><input id='mqttPass' name='mqttPass' type='password' value='" + mqttPass + "' placeholder='optional'></div></div>";
    page += "<div class='note'>MQTT data will be published only when the broker and topic are configured.</div></div>";
    page += "<div class='card'><h2>Station Status</h2>";
    page += "<div class='small'>AP IP Address: <strong>" + WiFi.softAPIP().toString() + "</strong></div>";
    if (WiFi.status() == WL_CONNECTED) {
        page += "<div class='small'>STA Status: <strong>Connected</strong> to <strong>" + WiFi.SSID() + "</strong> (RSSI <strong>" + String(WiFi.RSSI()) + " dBm</strong>)</div>";
        page += "<div class='small'>STA IP Address: <strong>" + WiFi.localIP().toString() + "</strong></div>";
    } else {
        page += "<div class='small'>STA Status: <strong>Disconnected</strong></div>";
    }
    page += "<div class='small'>WU Configured: <strong>" + String(wuID.length() > 0 ? "Yes" : "No") + "</strong></div>";
    page += "<div class='small'>APRS Configured: <strong>" + String(aprsCallsign.length() > 0 ? "Yes" : "No") + "</strong></div>";
    page += "</div>";
    page += "<div class='card'><h2>Weather Station</h2>";
    if (!weatherDataValid) {
        page += "<div class='small'>No weather data available yet.</div>";
    } else {
        page += "<div class='small'>Temperature: <strong>" + String(weatherData.tempC, 1) + "°C / " + String(weatherData.tempF, 1) + "°F</strong></div>";
        page += "<div class='small'>Humidity: <strong>" + String(weatherData.humidity, 0) + "%</strong></div>";
        page += "<div class='small'>Dew Point: <strong>" + String(weatherData.dewpointC, 1) + "°C / " + String(weatherData.dewpointF, 1) + "°F</strong></div>";
        page += "<div class='small'>Wind: <strong>" + String(weatherData.windSpeedMs, 1) + " m/s</strong> gust <strong>" + String(weatherData.windGustMs, 1) + " m/s</strong> (<strong>" + String(weatherData.windSpeedMph, 1) + " mph</strong> / <strong>" + String(weatherData.windGustMph, 1) + " mph</strong>)</div>";
        page += "<div class='small'>Wind Direction: <strong>" + String(weatherData.windDirection) + "°</strong></div>";
        page += "<div class='small'>Rain Hourly: <strong>" + String(weatherData.rainHourlyMM, 2) + " mm / " + String(weatherData.rainHourlyIn, 2) + " in</strong></div>";
        page += "<div class='small'>Rain Daily: <strong>" + String(weatherData.rainDailyMM, 2) + " mm / " + String(weatherData.rainDailyIn, 2) + " in</strong></div>";
        page += "<div class='small'>Precipitation Rate: <strong>" + String(weatherData.precipRateMMH, 2) + " mm/h / " + String(weatherData.precipRateInH, 3) + " in/h</strong></div>";
        if (weatherData.pressureHPa > 0) {
            page += "<div class='small'>Pressure: <strong>" + String(weatherData.pressureHPa, 1) + " hPa / " + String(weatherData.pressureInHg, 2) + " inHg</strong></div>";
        } else {
            page += "<div class='small'>Pressure: <strong>Not available</strong></div>";
        }
        page += "<div class='small'>Solar Radiation: <strong>" + String(weatherData.solarRadiation, 1) + " W/m²</strong></div>";
    }
    page += "</div>";
    page += "<div class='card'><h2>OTA Update</h2><div class='small'>Use the OTA page to upload new firmware.</div>";
    page += "<a class='button' href='http://" + ip.toString() + ":8080/update' target='_blank'>OTA Update</a></div>";
    page += "<div style='text-align:right; margin-top:12px;'><button class='button button-secondary' type='submit'>Save settings</button></div>";
    page += "</form></div></body></html>";
    return page;
}

void handleRootPage() {
    configServer.send(200, "text/html", buildConfigPage());
}

void handleSaveConfig() {
    if (configServer.hasArg("ssid")) wifiSSID = configServer.arg("ssid");
    if (configServer.hasArg("pass")) wifiPassword = configServer.arg("pass");
    if (configServer.hasArg("wuID")) wuID = configServer.arg("wuID");
    if (configServer.hasArg("wuKey")) wuKey = configServer.arg("wuKey");
    if (configServer.hasArg("aprsCall")) aprsCallsign = configServer.arg("aprsCall");
    if (configServer.hasArg("aprsPass")) aprsPassword = configServer.arg("aprsPass");
    if (configServer.hasArg("aprsLat")) aprsLatitude = atof(configServer.arg("aprsLat").c_str());
    if (configServer.hasArg("aprsLon")) aprsLongitude = atof(configServer.arg("aprsLon").c_str());
    if (configServer.hasArg("aprsComment")) aprsComment = configServer.arg("aprsComment");
    if (configServer.hasArg("mqttBroker")) mqttBroker = configServer.arg("mqttBroker");
    if (configServer.hasArg("mqttPort")) mqttPort = configServer.arg("mqttPort").toInt();
    if (configServer.hasArg("mqttTopic")) mqttTopic = configServer.arg("mqttTopic");
    if (configServer.hasArg("mqttUser")) mqttUser = configServer.arg("mqttUser");
    if (configServer.hasArg("mqttPass")) mqttPass = configServer.arg("mqttPass");

    prefs.putString("ssid", wifiSSID);
    prefs.putString("pass", wifiPassword);
    prefs.putString("wuID", wuID);
    prefs.putString("wuKey", wuKey);
    prefs.putString("aprsCall", aprsCallsign);
    prefs.putString("aprsPass", aprsPassword);
    prefs.putString("aprsLat", String(aprsLatitude, 4));
    prefs.putString("aprsLon", String(aprsLongitude, 4));
    prefs.putString("aprsComment", aprsComment);
    prefs.putString("mqttBroker", mqttBroker);
    prefs.putInt("mqttPort", mqttPort);
    prefs.putString("mqttTopic", mqttTopic);
    prefs.putString("mqttUser", mqttUser);
    prefs.putString("mqttPass", mqttPass);
    Serial.println("[CFG] Configuration saved");

    if (wifiSSID.length() > 0) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
        unsigned long start = millis();
        while (millis() - start < 8000 && WiFi.status() != WL_CONNECTED) {
            delay(200);
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WIFI] Reconnected to %s\n", wifiSSID.c_str());
        } else {
            Serial.println("[WIFI] Reconnect failed, still on AP");
        }
    }
    configServer.sendHeader("Location", "/", true);
    configServer.send(302, "text/plain", "");
}

void handleNotFound() {
    configServer.sendHeader("Location", "/", true);
    configServer.send(302, "text/plain", "");
}

void setupWebServer() {
    configServer.on("/", HTTP_GET, handleRootPage);
    configServer.on("/", HTTP_POST, handleSaveConfig);
    configServer.on("/save", HTTP_POST, handleSaveConfig);
    configServer.onNotFound(handleNotFound);
    configServer.begin();
    Serial.println("[WEB] Config server started on port 80");
}


void setupSensors() {
    Serial.println("[SENS] setupSensors: before ws.begin");
    int rc = ws.begin();
    Serial.printf("[SENS] setupSensors: after ws.begin rc=%d\n", rc);
    loraPresent = (rc == RADIOLIB_ERR_NONE);
    loraInitFailed = (rc != RADIOLIB_ERR_NONE);
    if (rc != RADIOLIB_ERR_NONE) {
        Serial.printf("[SENS] Radio init failed: %d\n", rc);
        Serial.println("[SENS] Radio receiver disabled");
    }

#ifdef USE_BMP280
    Serial.println("[SENS] setupSensors: before BMP280 init");
    Serial.printf("[SENS] BMP280 trying address 0x%02X\n", BMP280_I2C_ADDR_1);
    bmpPresent = bmp.begin(BMP280_I2C_ADDR_1);
    if (!bmpPresent) {
        Serial.printf("[SENS] BMP280 retry address 0x%02X\n", BMP280_I2C_ADDR_2);
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

#ifdef USE_SSD1306
void setupDisplay() {
    #ifdef OLED_SDA
    displayWire.begin(OLED_SDA, OLED_SCL, 400000UL);
    #else
    // I2C is initialized once in setup() for BMP280 and shared display usage.
    #endif

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("[OLED] SSD1306 init failed");
        displayPresent = false;
        return;
    }

    displayPresent = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("booting...");
    display.println("Bresser2WU");
    display.setTextSize(1);
    display.setCursor(0, 48);
    display.println("MajakOwO/Bresser2WU");
    display.display();
}

void renderDisplay(const WeatherSensor::Sensor& sensor, bool haveSensorData) {
    if (!displayPresent) {
        return;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);

    char line[64];
    if (!haveSensorData) {
        display.println("No weather data");
        if (WiFi.status() == WL_CONNECTED) {
            snprintf(line, sizeof(line), "IP:%s", WiFi.localIP().toString().c_str());
        } else {
            snprintf(line, sizeof(line), "WiFi: disconnected");
        }
        display.println(line);
    } else {
        snprintf(line, sizeof(line), "T:%4.1fC H:%3.0f%%", weatherData.tempC, weatherData.humidity);
        display.println(line);

        snprintf(line, sizeof(line), "DP:%3.0fC P:%4.0fhPa", weatherData.dewpointC, weatherData.pressureHPa > 0 ? weatherData.pressureHPa : 0.0f);
        display.println(line);

        snprintf(line, sizeof(line), "W:%4.1f/%4.1f m/s D:%3d", weatherData.windSpeedMs, weatherData.windGustMs, weatherData.windDirection);
        display.println(line);

        snprintf(line, sizeof(line), "Rain:%4.1fmm 24h:%4.1f", weatherData.rainDailyMM, weatherData.rainHourlyMM);
        display.println(line);

        snprintf(line, sizeof(line), "Rate:%4.1fmm/h SR:%4.0f", weatherData.precipRateMMH, weatherData.solarRadiation);
        display.println(line);

        String statusLine;
        if (!wifiConfigured) {
            statusLine = "NO WIFI";
        } else if (!wifiConnected) {
            statusLine = "WIFI FAILED";
        }
        if (!loraPresent) {
            if (statusLine.length()) {
                display.println(statusLine);
                statusLine = "";
            }
            display.println("LoRa ERROR");
        }
#ifdef USE_BMP280
        if (!bmpPresent) {
            if (statusLine.length()) {
                display.println(statusLine);
                statusLine = "";
            }
            display.println("BMP280 OFF");
        }
#endif
        if (statusLine.length() == 0 && wifiConfigured && wifiConnected && loraPresent
#ifdef USE_BMP280
            && bmpPresent
#endif
        ) {
            snprintf(line, sizeof(line), "Status: OK");
            display.println(line);
        } else if (statusLine.length()) {
            statusLine.trim();
            statusLine.toCharArray(line, sizeof(line));
            display.println(line);
        }

        if (WiFi.status() == WL_CONNECTED) {
            snprintf(line, sizeof(line), "IP:%s", WiFi.localIP().toString().c_str());
        } else {
            snprintf(line, sizeof(line), "WiFi: disconnected");
        }
        display.println(line);
    }

    display.display();
}
#else
void setupDisplay() {}
void renderDisplay(const WeatherSensor::Sensor& sensor, bool haveSensorData) {}
#endif

#ifdef TEST_WEATHER
void generateTestWeatherData() {
    weatherData.tempC = 20.3f;
    weatherData.tempF = 68.54f;
    weatherData.humidity = 45.0f;
    weatherData.dewpointC = 7.970911f;
    weatherData.dewpointF = 46.34764f;
    weatherData.windSpeedMs = 1.7f;
    weatherData.windGustMs = 1.8f;
    weatherData.windSpeedMph = 3.802798f;
    weatherData.windGustMph = 4.026492f;
    weatherData.windDirection = 138;
    weatherData.rainHourlyMM = 0.0f;
    weatherData.rainHourlyIn = 0.0f;
    weatherData.rainDailyMM = 0.0f;
    weatherData.rainDailyIn = 0.0f;
    weatherData.precipRateMMH = 0.0f;
    weatherData.precipRateInH = 0.0f;
    weatherData.pressureHPa = 978.83f;
    weatherData.pressureInHg = 28.90483f;
    weatherData.solarRadiation = 819.4276f;
}
#endif

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.printf("Starting execution...\n");

#ifdef USE_BMP280
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000UL);
    Serial.printf("[I2C] Initialized I2C on SDA=%d SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("[I2C] Scanning for devices...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("[I2C] Device found at 0x%02X\n", addr);
        }
    }
#endif

    initBoard();
    setupPreferences();
    mqttClient.setBufferSize(512);
#ifdef USE_SSD1306
    setupDisplay();
#endif

    if (!connectSavedWiFi()) {
        startConfigAP();
    }

    setupWebServer();

    otaUpdater.setup(&otaServer, "/update");
    otaServer.begin();
    Serial.printf("[OTA] Update server available at http://%s:8080/update\n", WiFi.localIP().toString().c_str());

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

bool setupMQTT() {
    if (mqttBroker.length() == 0 || mqttTopic.length() == 0) {
        return false;
    }

    mqttClient.setServer(mqttBroker.c_str(), mqttPort);
    if (!mqttClient.connected()) {
        Serial.printf("[MQTT] Connecting to %s:%u\n", mqttBroker.c_str(), mqttPort);
        bool connected;
        if (mqttUser.length() > 0) {
            connected = mqttClient.connect("Bresser2WU", mqttUser.c_str(), mqttPass.c_str());
        } else {
            connected = mqttClient.connect("Bresser2WU");
        }
        if (connected) {
            Serial.println("[MQTT] Connected");
        } else {
            Serial.printf("[MQTT] Connect failed, rc=%d\n", mqttClient.state());
            return false;
        }
    }
    return true;
}

bool publishMQTT(const char* payload) {
    if (mqttBroker.length() == 0 || mqttTopic.length() == 0) {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT] WiFi nie jest połączone, pomijam publikację");
        return false;
    }

    if (!mqttClient.connected()) {
        if (!setupMQTT()) {
            return false;
        }
    }

    bool ok = mqttClient.publish(mqttTopic.c_str(), payload);
    Serial.printf("[MQTT] Publish %s: %s\n", ok ? "OK" : "FAILED", payload);
    return ok;
}

void sendToMQTT() {
    if (!weatherDataValid) {
        return;
    }

    StaticJsonDocument<512> json;
    json["temperature_c"] = weatherData.tempC;
    json["temperature_f"] = weatherData.tempF;
    json["humidity"] = weatherData.humidity;
    json["dewpoint_c"] = weatherData.dewpointC;
    json["dewpoint_f"] = weatherData.dewpointF;
    json["wind_speed_ms"] = weatherData.windSpeedMs;
    json["wind_gust_ms"] = weatherData.windGustMs;
    json["wind_speed_mph"] = weatherData.windSpeedMph;
    json["wind_gust_mph"] = weatherData.windGustMph;
    json["wind_direction"] = weatherData.windDirection;
    json["rain_hourly_mm"] = weatherData.rainHourlyMM;
    json["rain_hourly_in"] = weatherData.rainHourlyIn;
    json["rain_daily_mm"] = weatherData.rainDailyMM;
    json["rain_daily_in"] = weatherData.rainDailyIn;
    json["precip_rate_mmh"] = weatherData.precipRateMMH;
    json["precip_rate_inh"] = weatherData.precipRateInH;
    json["pressure_hpa"] = weatherData.pressureHPa;
    json["pressure_inhg"] = weatherData.pressureInHg;
    json["solar_radiation_wpm2"] = weatherData.solarRadiation;

    char payload[768];
    serializeJson(json, payload, sizeof(payload));
    publishMQTT(payload);
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
    float currentRainMM = sensor.w.rain_mm;

    // Total rain in the last ~60 minutes (derived from history)
    weatherData.rainHourlyMM = 0;
    if (rainBufferFilled) {
        weatherData.rainHourlyMM = currentRainMM - rainHistory[rainIndex].rainMM;
    } else if (rainIndex > 0) {
        weatherData.rainHourlyMM = currentRainMM - rainHistory[0].rainMM;
    }
    if (weatherData.rainHourlyMM < 0) weatherData.rainHourlyMM = 0;
    weatherData.rainHourlyIn = weatherData.rainHourlyMM / 25.4;

    // Daily rain
    weatherData.rainDailyMM = currentRainMM - rainAtMidnightMM;
    if (weatherData.rainDailyMM < 0) weatherData.rainDailyMM = 0;
    weatherData.rainDailyIn = weatherData.rainDailyMM / 25.4;

    // Precipitation rate = delta rain counter / delta time
    // Only update the value every PRECIP_RATE_UPDATE_INTERVAL to avoid erratic jumps.
    // Keep the last computed rate between updates.
    unsigned long nowMs = millis();
    if (lastRainRateTsMillis == 0 || lastRainRateRainMM < 0) {
        // Initialize on first call
        lastRainRateTsMillis = nowMs;
        lastRainRateRainMM = currentRainMM;
        weatherData.precipRateMMH = 0;
        weatherData.precipRateInH = 0;
    } else if (nowMs - lastRainRateTsMillis >= PRECIP_RATE_UPDATE_INTERVAL) {
        unsigned long dtMs = nowMs - lastRainRateTsMillis; // unsigned handles wrap-around
        float dtHours = dtMs / 3600000.0f;
        float deltaRainMM = currentRainMM - lastRainRateRainMM;

        // Counter rollover/reset handling
        if (deltaRainMM < 0) {
            deltaRainMM = 0;
        }

        if (dtHours > 0.00001f) {
            weatherData.precipRateMMH = deltaRainMM / dtHours;
            if (weatherData.precipRateMMH < 0) weatherData.precipRateMMH = 0;
            weatherData.precipRateInH = weatherData.precipRateMMH / 25.4;
        }

        lastRainRateTsMillis = nowMs;
        lastRainRateRainMM = currentRainMM;
    }
}

// Service Transmission Functions

void sendToWU() {
    if (wuID.length() == 0 || wuKey.length() == 0) {
        return;  // WU not configured
    }

    if (WiFi.status() != WL_CONNECTED) return;

    int const i = 0;
#ifdef TEST_WEATHER
    // Test mode uses synthetic weather data only.
    float currentRain = rainAtMidnightMM;
#else
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
#endif

    // Initialize precip rate baseline on first WU send
    if (lastRainRateTsMillis == 0 || lastRainRateRainMM < 0) {
        lastRainRateTsMillis = millis();
        lastRainRateRainMM = currentRain;
    }

    // Weather data already calculated in loop()
    
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
        "&rainin=" + String(weatherData.precipRateInH, 3) +
        "&dailyrainin=" + String(weatherData.rainDailyIn, 3) +

#ifdef TEST_WEATHER
        "&UV=0" +
#else
        "&UV=" + String(ws.sensor[i].w.uv, 1) +
#endif

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
#ifndef TEST_WEATHER
    if (!ws.sensor[i].w.temp_ok) return false;

    // Keep rain history updated even when only APRS is enabled
    updateRainCounters();
    float currentRain = ws.sensor[i].w.rain_mm;
    updateRainHistory(currentRain);
    if (currentRain < rainAtMidnightMM) {
        rainAtMidnightMM = currentRain;
        prefs.putFloat("rainMid", rainAtMidnightMM);
    }
#endif

    // Connect if needed
    if (!aprs.isConnected()) {
        if (!aprs.connectToServer()) {
            Serial.println("[APRS] Connection attempt failed");
            return false;
        }
    }

    // Weather data already calculated in loop()

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
#ifdef TEST_WEATHER
        true,
        -50
#else
        ws.sensor[i].battery_ok,
        ws.sensor[i].rssi
#endif
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

    // Process OTA server and custom config web server requests
    otaServer.handleClient();
    configServer.handleClient();

#ifdef TEST_WEATHER
    bool haveSensorData = true;
#else
    ws.clearSlots();
    int decode_status = ws.getMessage();

    bool haveSensorData = ws.sensor[i].w.temp_ok;

    if (decode_status == DECODE_OK) {
        printSensorInfo(ws.sensor[i]);
    }
#endif

#ifdef USE_SSD1306
    renderDisplay(ws.sensor[i], haveSensorData);
#endif

    if (haveSensorData) {
#ifdef TEST_WEATHER
        generateTestWeatherData();
#else
        // Calculate weather data and print precipitation rate
        calculateWeatherData(ws.sensor[i]);
#endif
        weatherDataValid = true;
        Serial.printf("Precip: [%6.2f mm/h] [%6.3f in/h]\n", weatherData.precipRateMMH, weatherData.precipRateInH);
        
        if (millis() - lastWU > WU_UPDATE_INTERVAL) {
            sendToWU();
            lastWU = millis();
            sendToMQTT();
        }
        if (mqttClient.connected()) {
            mqttClient.loop();
        }

        if (millis() - lastAPRS > APRS_UPDATE_INTERVAL) {
            if (sendToAPRS()) {
                lastAPRS = millis();
            }
        }
    }

    delay(100);
}
