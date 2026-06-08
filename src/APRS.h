///////////////////////////////////////////////////////////////////////////////////////////////////
// APRS.h
//
// APRS-IS (Automatic Packet Reporting System - Internet Service) Handler
// Sends weather station data to APRS-IS network via TCP
//
// Created: 2025
//
// MIT License
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef APRS_H
#define APRS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

class APRS {
private:
    WiFiClient client;
    const char* aprsServer;
    const uint16_t aprsPort;
    bool connected;
    unsigned long lastConnectionAttempt;
    const unsigned long connectionRetryInterval = 300000; // 5 minutes
    
    String callsign;
    String password;
    String comment;
    float latitude;
    float longitude;
    int weatherSendCount;
    
    // Helper methods
    void sendLoginString();
    String formatWeatherData(float tempF,
                             float humidity,
                             float windSpeedMs,
                             float windGustMs,
                             int windDirection,
                             float rainfallHourly,
                             float rainfallDaily,
                             float pressureInHg,
                             float solarRadiation,
                             bool batteryOk,
                             float rssiDbm);

public:
    APRS();
    ~APRS();
    
    // Initialize with callsign, password, and coordinates
    void begin(const String& callSign, const String& pwd, float lat, float lon);
    
    // Send weather data to APRS-IS
    bool sendWeatherData(float tempF, float humidity, float windSpeedMs, 
                        float windGustMs, int windDirection, float rainfallHourly,
                        float rainfallDaily,
                        float pressureInHg, float solarRadiation, 
                        bool batteryOk, float rssiDbm);
    
    // Check and maintain connection
    bool isConnected();
    void disconnect();
    void update();
    bool connectToServer();  // Made public for connection management
    
    // Set optional parameters
    void setComment(const String& comment);
    // Send an APRS status message (payload starting with '>')
    bool sendStatus(const String& statusText);
    

};

#endif // APRS_H
