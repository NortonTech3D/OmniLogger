/*
 * OmniLogger - Semi-Universal Data Logger
 * For Lolin (WEMOS) ESP32-S2 Mini
 * 
 * Copyright (C) 2024 NortonTech3D
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * 
 * Features:
 * - Multi-sensor support (BME280, DHT22, DS18B20, Analog)
 * - SD card data logging
 * - Web interface for configuration and monitoring
 * - NTP time synchronization
 * - Battery monitoring
 * - Deep sleep power management
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <time.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "sensors.h"
#include "webserver.h"
#include "datalogger.h"

// Configuration stored in EEPROM/Flash
Config deviceConfig;
SensorManager sensorManager;
DataLogger dataLogger;
WebServerManager webServer;

// Global state
unsigned long lastMeasurement = 0;
unsigned long bootTime = 0;
uint32_t measurementCount = 0;
bool timeInitialized = false;

// Measurement count persistence
Preferences measurementPrefs;

// WiFi management
bool wifiEnabled = true;
unsigned long wifiTimeoutStart = 0;
const unsigned long WIFI_TIMEOUT_MS = 180000;  // 3 minutes
volatile bool wifiReenableRequested = false;

// Forward declarations
void setupWiFi();
void syncTime();
void takeMeasurement();
void enterDeepSleep();
float readBatteryVoltage();
void checkWiFiTimeout();
void disableWiFi();
void enableWiFi();
void IRAM_ATTR wifiButtonISR();

// WiFi status getter for web interface
bool getWiFiEnabled() {
  return wifiEnabled;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("OmniLogger - Data Logger Starting...");
  Serial.println("========================================");
  
  bootTime = millis();
  
  // Initialize measurement count from NVS
  if (measurementPrefs.begin("measurements", false)) {
    measurementCount = measurementPrefs.getUInt("count", 0);
    Serial.printf("Restored measurement count: %d\n", measurementCount);
  }
  
  // Initialize configuration
  if (!deviceConfig.begin()) {
    Serial.println("ERROR: Failed to initialize configuration!");
    Serial.println("Using default configuration...");
  }
  deviceConfig.load();
  
  // Initialize LittleFS for web files
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: Failed to mount LittleFS!");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
  
  // Initialize DataLogger (SD card will be lazy-initialized on first flush)
  pinMode(deviceConfig.sdCardCS, OUTPUT);
  digitalWrite(deviceConfig.sdCardCS, HIGH);
  
  if (!dataLogger.begin(deviceConfig.sdCardCS)) {
    Serial.println("WARNING: DataLogger initialization failed!");
  } else {
    Serial.println("DataLogger initialized successfully");
  }
  
  // Configure data buffering
  dataLogger.setBufferingEnabled(deviceConfig.bufferingEnabled);
  if (deviceConfig.bufferingEnabled) {
    Serial.printf("Data buffering enabled with %d second flush interval\n", deviceConfig.flushInterval);
  }
  
  // Initialize sensors
  sensorManager.begin(deviceConfig);
  Serial.printf("Initialized %d sensors\n", sensorManager.getSensorCount());
  
  // Setup WiFi
  setupWiFi();
  
  // Setup GPIO 0 button for WiFi re-enable
  pinMode(0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), wifiButtonISR, FALLING);
  Serial.println("GPIO 0 button configured for WiFi re-enable");
  
  // Start WiFi timeout timer
  wifiTimeoutStart = millis();
  
  // Sync time from NTP
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
  }
  
  // Start web server
  webServer.begin(&deviceConfig, &sensorManager, &dataLogger, readBatteryVoltage, getWiFiEnabled);
  Serial.println("Web server started");
  
  Serial.println("Setup complete!");
  Serial.println("========================================\n");
}

void loop() {
  // Check for WiFi re-enable request from button
  if (wifiReenableRequested) {
    wifiReenableRequested = false;
    enableWiFi();
  }
  
  // Handle web server if WiFi is enabled
  if (wifiEnabled) {
    webServer.handleClient();
    checkWiFiTimeout();
  }
  
  // Check if buffered data should be flushed
  if (deviceConfig.bufferingEnabled && 
      dataLogger.shouldFlush(deviceConfig.flushInterval * 1000UL)) {
    Serial.println("Periodic buffer flush triggered");
    dataLogger.flushBuffer();
  }
  
  // Check if it's time for a measurement
  unsigned long currentTime = millis();
  if (currentTime - lastMeasurement >= deviceConfig.measurementInterval * 1000UL) {
    takeMeasurement();
    lastMeasurement = currentTime;
    
    // If deep sleep is enabled and we're battery powered, enter sleep
    if (deviceConfig.deepSleepEnabled && readBatteryVoltage() < 5.0) {
      enterDeepSleep();
    }
  }
  
  // Resync time periodically (every 12 hours)
  static unsigned long lastTimeSync = 0;
  if (timeInitialized && wifiEnabled && millis() - lastTimeSync > 12 * 60 * 60 * 1000UL) {
    syncTime();
    lastTimeSync = millis();
  }
  
  delay(10);
}

void setupWiFi() {
  Serial.println("\nConfiguring WiFi...");
  
  if (strlen(deviceConfig.wifiSSID) > 0) {
    // Try to connect to configured WiFi
    Serial.printf("Connecting to WiFi: %s\n", deviceConfig.wifiSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(deviceConfig.wifiSSID, deviceConfig.wifiPassword);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return;
    } else {
      Serial.println("Failed to connect to WiFi");
    }
  }
  
  // Start in AP mode if no WiFi or connection failed
  Serial.println("Starting Access Point mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(deviceConfig.apSSID, deviceConfig.apPassword);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void syncTime() {
  Serial.println("Syncing time from NTP...");
  
  configTime(deviceConfig.timezoneOffset * 3600, 0, 
             "pool.ntp.org", "time.nist.gov", "time.google.com");
  
  // Wait for time to be set
  int attempts = 0;
  time_t now = 0;
  struct tm timeinfo = {0};
  
  while (attempts < 10) {
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > (2020 - 1900)) {
      break;
    }
    delay(500);
    attempts++;
  }
  
  if (timeinfo.tm_year > (2020 - 1900)) {
    timeInitialized = true;
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("Time synchronized: %s\n", buffer);
  } else {
    Serial.println("Failed to sync time from NTP");
    timeInitialized = false;
  }
}

void takeMeasurement() {
  Serial.println("\n--- Taking Measurement ---");
  
  // Get current time
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  
  char timestamp[32];
  if (timeInitialized) {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
  } else {
    snprintf(timestamp, sizeof(timestamp), "UTC+%ld", now);
  }
  
  // Read all sensors
  sensorManager.readAllSensors();
  
  // Write header for new files
  dataLogger.writeHeader(sensorManager.getCSVHeader());
  
  // Log to SD card
  String logEntry = sensorManager.getCSVData(timestamp);
  if (dataLogger.logData(logEntry)) {
    measurementCount++;
    measurementPrefs.putUInt("count", measurementCount);
    Serial.printf("Data logged successfully (count: %d)\n", measurementCount);
  } else {
    Serial.println("Failed to log data");
  }
  
  // Print to serial
  Serial.printf("Timestamp: %s\n", timestamp);
  sensorManager.printReadings();
  Serial.printf("Battery: %.2fV\n", readBatteryVoltage());
  Serial.println("--- Measurement Complete ---\n");
}

void enterDeepSleep() {
  Serial.printf("Entering deep sleep for %d seconds...\n", deviceConfig.measurementInterval);
  
  // Do NOT flush buffer - it persists in NVS across deep sleep
  // Buffer will be flushed when threshold is reached or manually triggered
  
  // Configure wake up
  esp_sleep_enable_timer_wakeup(deviceConfig.measurementInterval * 1000000ULL);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

float readBatteryVoltage() {
  // ESP32-S2 has ADC on GPIO1-10
  // Using configured GPIO for battery voltage measurement with voltage divider
  // Assuming 2:1 voltage divider (battery max 4.2V -> ADC max 2.1V)
  // ADC reads 0-4095 for 0-3.3V range
  
  const float adcMax = 4095.0;
  const float adcVoltage = 3.3;
  const float voltageDivider = 2.0;
  
  // Validate battery pin is configured
  if (deviceConfig.batteryPin < 0 || deviceConfig.batteryPin > 10) {
    Serial.println("Warning: Invalid battery pin configured");
    return 0.0;
  }
  
  int rawValue = analogRead(deviceConfig.batteryPin);
  
  // Validate ADC reading
  if (rawValue < 0 || rawValue > 4095) {
    Serial.printf("Warning: Invalid ADC reading: %d\n", rawValue);
    return 0.0;
  }
  
  float voltage = (rawValue / adcMax) * adcVoltage * voltageDivider;
  
  // Sanity check: typical LiPo range is 2.5V - 4.2V, with 2:1 divider ADC sees 1.25V - 2.1V
  // Code multiplies by 2 to calculate actual battery voltage (2.5V - 4.2V)
  // If reading seems wrong, it might be USB powered (showing ~5V from USB)
  return voltage;
}

void checkWiFiTimeout() {
  if (!wifiEnabled) return;
  
  unsigned long currentTime = millis();
  unsigned long elapsed;
  
  // Handle millis() rollover
  if (currentTime >= wifiTimeoutStart) {
    elapsed = currentTime - wifiTimeoutStart;
  } else {
    elapsed = (0xFFFFFFFF - wifiTimeoutStart) + currentTime + 1;
  }
  
  // Check timeout
  if (elapsed >= WIFI_TIMEOUT_MS) {
    // Check if we have clients or connection
    bool hasActivity = false;
    
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
      // AP mode - check for connected clients
      if (WiFi.softAPgetStationNum() > 0) {
        hasActivity = true;
        wifiTimeoutStart = currentTime;  // Reset timer
      }
    }
    
    if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
      // STA mode - check for connection
      if (WiFi.status() == WL_CONNECTED) {
        hasActivity = true;
        wifiTimeoutStart = currentTime;  // Reset timer
      }
    }
    
    // If no activity after timeout, disable WiFi
    if (!hasActivity) {
      Serial.println("WiFi timeout reached with no activity - disabling WiFi");
      disableWiFi();
    }
  }
}

void disableWiFi() {
  if (!wifiEnabled) return;
  
  Serial.println("Disabling WiFi and web server to save power...");
  
  // Stop web server (there's no explicit stop method, but disconnecting WiFi will stop requests)
  
  // Disconnect and disable WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  wifiEnabled = false;
  
  Serial.println("WiFi disabled. Press GPIO 0 button to re-enable.");
}

void enableWiFi() {
  if (wifiEnabled) return;
  
  Serial.println("Re-enabling WiFi...");
  
  // Restart WiFi setup
  setupWiFi();
  
  // Restart web server
  webServer.begin(&deviceConfig, &sensorManager, &dataLogger, readBatteryVoltage, getWiFiEnabled);
  
  // Reset timeout timer
  wifiTimeoutStart = millis();
  wifiEnabled = true;
  
  // Sync time if connected
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
  }
  
  Serial.println("WiFi re-enabled successfully");
}

void IRAM_ATTR wifiButtonISR() {
  // Set flag to re-enable WiFi in main loop (can't do WiFi operations in ISR)
  wifiReenableRequested = true;
}
