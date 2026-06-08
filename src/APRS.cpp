///////////////////////////////////////////////////////////////////////////////////////////////////
// APRS.cpp
//
// APRS-IS (Automatic Packet Reporting System - Internet Service) Handler
// Sends weather station data to APRS-IS network via TCP
//
// Created: 2025
//
// MIT License
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "APRS.h"
#include <WiFi.h>

APRS::APRS() : 
    aprsServer("rotate.aprs.net"),
    aprsPort(14580),
    connected(false),
    lastConnectionAttempt(0),
    latitude(0.0),
    longitude(0.0) {
    weatherSendCount = 0;
}

APRS::~APRS() {
    if (connected) {
        client.stop();
    }
}

void APRS::begin(const String& callSign, const String& pwd, float lat, float lon) {
    callsign = callSign;
    password = pwd;
    latitude = lat;
    longitude = lon;
    comment = "";
    
    Serial.println("[APRS] Initialized");
    Serial.printf("[APRS] Callsign: %s\n", callsign.c_str());
    Serial.printf("[APRS] Position: %.4f, %.4f\n", latitude, longitude);
}

void APRS::setComment(const String& cmt) {
    comment = cmt;
    Serial.printf("[APRS] Comment set: %s\n", comment.c_str());
}



bool APRS::connectToServer() {
    if (connected) {
        return true;
    }
    
    // Don't retry too frequently (but allow first attempt immediately)
    if (lastConnectionAttempt != 0) {
        unsigned long since = millis() - lastConnectionAttempt;
        if (since < connectionRetryInterval) {
            unsigned long waitMs = connectionRetryInterval - since;
            Serial.printf("[APRS] Skipping connect: retry in %lus\n", waitMs / 1000);
            return false;
        }
    }
    
    lastConnectionAttempt = millis();
    
    Serial.printf("[APRS] Connecting to %s:%d\n", aprsServer, aprsPort);

    // Ensure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[APRS] WiFi not connected, cannot connect to APRS server");
        lastConnectionAttempt = millis();
        return false;
    }

    // Check WiFi
    int wifiStatus = (int)WiFi.status();
    Serial.printf("[APRS] WiFi status: %d\n", wifiStatus);

    // Try DNS resolution first and show resolved IP for diagnostics
    IPAddress srvIP;
    bool resolved = false;
    if (WiFi.hostByName(aprsServer, srvIP)) {
        Serial.printf("[APRS] Resolved %s -> %s\n", aprsServer, srvIP.toString().c_str());
        resolved = true;
    } else {
        Serial.printf("[APRS] DNS resolution failed for %s\n", aprsServer);
    }

    bool connectedNow = false;
    if (resolved) {
        connectedNow = client.connect(srvIP, aprsPort);
        if (!connectedNow) {
            Serial.printf("[APRS] Connect to IP %s failed\n", srvIP.toString().c_str());
        }
    }

    // Fallback: try connecting by hostname if IP attempt failed
    if (!connectedNow) {
        Serial.printf("[APRS] Trying connect using hostname %s\n", aprsServer);
        connectedNow = client.connect(aprsServer, aprsPort);
    }

    if (!connectedNow) {
        Serial.println("[APRS] Connection failed");
        return false;
    }
    
    Serial.println("[APRS] Connected!");
    
    // Send login string
    sendLoginString();
    connected = true;
    
    return true;
}

void APRS::sendLoginString() {
    // APRS-IS login format: user <callsign> pass <password> vers <software> <version>
    String loginStr = "user " + callsign + " pass " + password + " vers Bresser2APRS 1.0\n";
    
    client.print(loginStr);
    Serial.printf("[APRS] Sent login: user %s pass ***** vers Bresser2APRS 1.0\n", callsign.c_str());
    
    // Wait a bit for server response
    delay(500);
    while (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.printf("[APRS] Server: %s\n", line.c_str());
    }
}

String APRS::formatWeatherData(float tempF,
                                  float humidity,
                                  float windSpeedMs,
                                  float windGustMs,
                                  int windDirection,
                                  float rainfallHourly,
                                  float rainfallDaily,
                                  float pressureHPa,
                                  float solarRadiation,
                                  bool batteryOk,
                                  float rssiDbm) {
    // APRS weather format (APRS spec WX.TXT / spec-wx.txt):
    // !DDMM.hhN/DDDMM.hhW_ddd/sssgggtXXXhXXPXXXXXrXXXpXXX
    // Where:
    // _ddd = wind direction (0-360 degrees)
    // /sss = sustained 1-minute wind speed (m/s)
    // ggg = peak/gust wind speed (m/s)
    // tXXX = temperature in Fahrenheit (-99 to +150)
    // hXX = humidity in percent (00-99, where 00 = 100%)
    // PXXXXX = barometric pressure in tenths of millibars (e.g. P10125 = 1012.5 mb)
    // rXXX = rainfall in last hour (in 0.01" increments)
    // pXXX = rainfall in last 24 hours (in 0.01" increments)
    
    // Convert coordinates to APRS format (degrees minutes, hundredths)
    int latDeg = (int)abs(latitude);
    int latMinHundredths = static_cast<int>(((abs(latitude) - latDeg) * 60.0f) * 100.0f + 0.5f);
    if (latMinHundredths >= 6000) {
        latMinHundredths -= 6000;
        latDeg++;
    }
    int latMin = latMinHundredths / 100;
    int latMinFrac = latMinHundredths % 100;
    char latChar = latitude >= 0 ? 'N' : 'S';

    int lonDeg = (int)abs(longitude);
    int lonMinHundredths = static_cast<int>(((abs(longitude) - lonDeg) * 60.0f) * 100.0f + 0.5f);
    if (lonMinHundredths >= 6000) {
        lonMinHundredths -= 6000;
        lonDeg++;
    }
    int lonMin = lonMinHundredths / 100;
    int lonMinFrac = lonMinHundredths % 100;
    char lonChar = longitude >= 0 ? 'E' : 'W';
    
    // Format APRS position and weather string
    char aprsData[256];
    
    // APRS packet format: !DDMM.hhN/DDDMM.hhW_ddd/sssgggtXXXhXXPXXXXXrXXXpXXX
    
    // Humidity: 00=100%, 01-99=percent
    int humidity_aprs = (int)humidity;
    if (humidity_aprs >= 100) humidity_aprs = 0;

    int rainHourlyHundredths = static_cast<int>(rainfallHourly * 100.0f + 0.5f);
    if (rainHourlyHundredths < 0) rainHourlyHundredths = 0;
    if (rainHourlyHundredths > 999) rainHourlyHundredths = 999;

    int rainDailyHundredths = static_cast<int>(rainfallDaily * 100.0f + 0.5f);
    if (rainDailyHundredths < 0) rainDailyHundredths = 0;
    if (rainDailyHundredths > 999) rainDailyHundredths = 999;

    int pressureTenthsMb = static_cast<int>(pressureHPa * 10.0f + 0.5f);
    if (pressureTenthsMb < 0) pressureTenthsMb = 0;
    if (pressureTenthsMb > 99999) pressureTenthsMb = 99999;
    
    snprintf(aprsData, sizeof(aprsData),
        "!%02d%02d.%02d%c/%03d%02d.%02d%c_%03d/%03dg%03dt%03dh%02dP%05dr%03dp%03d",
        latDeg, latMin, latMinFrac, latChar,
        lonDeg, lonMin, lonMinFrac, lonChar,
        (int)windDirection,
        (int)windSpeedMs,
        (int)windGustMs,
        (int)tempF,
        humidity_aprs,
        pressureTenthsMb,
        rainHourlyHundredths,
        rainDailyHundredths
    );

    String result = String(aprsData);
    if (comment.length() > 0) {
        result += " ";
        result += comment;
    }
    result += batteryOk ? " BATT [OK]" : " BATT [LOW]";
    result += " RSSI=" + String((int)rssiDbm) + "dBm";

    return result;
}

bool APRS::sendWeatherData(float tempF, float humidity, float windSpeedMs, 
                          float windGustMs, int windDirection, float rainfallHourly,
                          float rainfallDaily,
                          float pressureHPa, float solarRadiation,
                          bool batteryOk, float rssiDbm) {
    if (!connectToServer()) {
        return false;
    }
    
    // Format the weather data as APRS packet
    String weatherData = formatWeatherData(tempF, humidity, windSpeedMs, windGustMs, 
                                          windDirection, rainfallHourly, rainfallDaily, pressureHPa, solarRadiation,
                                          batteryOk, rssiDbm);
    
    // APRS packet format: callsign>APRS,qAS,SOURCE:/weather_data
    String aprsPacket = callsign + ">APRS,qAS," + callsign + ":" + weatherData + "\n";
    
    // Send the packet
    if (client.print(aprsPacket)) {
        Serial.printf("[APRS] Sent: %s", aprsPacket.c_str());
        // Increment weather send counter and send status every 10 frames
        weatherSendCount++;
        if ((weatherSendCount % 10) == 0) {
            String st = String("System status");
            st += batteryOk ? " BATT [OK]" : " BATT [LOW]";
            st += " RSSI=" + String((int)rssiDbm) + "dBm";
            sendStatus(st);
            // Send repository link as a separate status packet so it appears on next line
            sendStatus("https://github.com/MajakOwO/Bresser2WU");
        }
        return true;
    } else {
        Serial.println("[APRS] Failed to send packet");
        connected = false;
        client.stop();
        // allow immediate reconnect attempt
        lastConnectionAttempt = 0;
        return false;
    }
}

bool APRS::sendStatus(const String& statusText) {
    if (!connectToServer()) {
        return false;
    }

    // Ensure status payload starts with '>' per APRS status frame convention
    String payload = statusText;
    if (payload.length() == 0) return false;
    if (payload.charAt(0) != '>') {
        payload = ">" + payload;
    }

    // Ensure no embedded newlines in payload (APRS frames are single-line)
    for (int i = 0; i < payload.length(); ++i) {
        if (payload.charAt(i) == '\n' || payload.charAt(i) == '\r') {
            payload.setCharAt(i, ' ');
        }
    }

    String aprsPacket = callsign + ">APRS,qAS," + callsign + ":" + payload + "\n";

    if (client.print(aprsPacket)) {
        Serial.printf("[APRS] Sent status: %s", aprsPacket.c_str());
        return true;
    } else {
        Serial.println("[APRS] Failed to send status");
        connected = false;
        client.stop();
        lastConnectionAttempt = 0;
        return false;
    }
}

bool APRS::isConnected() {
    if (!connected) {
        return false;
    }
    
    if (!client.connected()) {
        Serial.println("[APRS] Connection lost");
        connected = false;
        // allow immediate reconnect attempt
        lastConnectionAttempt = 0;
        return false;
    }
    
    return true;
}

void APRS::disconnect() {
    if (connected) {
        client.stop();
        connected = false;
        Serial.println("[APRS] Disconnected");
    }
}

void APRS::update() {
    // This can be used to read server responses or maintain connection
    if (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.length() > 0) {
            Serial.printf("[APRS] Server response: %s\n", line.c_str());
        }
    }
}
