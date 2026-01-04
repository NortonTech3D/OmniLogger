/*
 * OmniLogger - Semi-Universal Data Logger
 * For Lolin (WEMOS) ESP32-S2 Mini
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

// Forward declarations
void setupWiFi();
void syncTime();
void takeMeasurement();
void enterDeepSleep();
float readBatteryVoltage();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("OmniLogger - Data Logger Starting...");
  Serial.println("========================================");
  
  bootTime = millis();
  
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
  
  // Initialize SD card
  pinMode(deviceConfig.sdCardCS, OUTPUT);
  digitalWrite(deviceConfig.sdCardCS, HIGH);
  
  if (!dataLogger.begin(deviceConfig.sdCardCS)) {
    Serial.println("WARNING: SD card initialization failed!");
  } else {
    Serial.println("SD card initialized successfully");
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
  
  // Sync time from NTP
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
  }
  
  // Start web server
  webServer.begin(&deviceConfig, &sensorManager, &dataLogger);
  Serial.println("Web server started");
  
  Serial.println("Setup complete!");
  Serial.println("========================================\n");
}

void loop() {
  // Handle web server
  webServer.handleClient();
  
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
  if (timeInitialized && millis() - lastTimeSync > 12 * 60 * 60 * 1000UL) {
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
  
  // Log to SD card
  String logEntry = sensorManager.getCSVData(timestamp);
  if (dataLogger.logData(logEntry)) {
    measurementCount++;
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
  
  // Close files and cleanup
  dataLogger.flush();
  
  // Configure wake up
  esp_sleep_enable_timer_wakeup(deviceConfig.measurementInterval * 1000000ULL);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

float readBatteryVoltage() {
  // ESP32-S2 has ADC on GPIO1-10
  // Using GPIO1 for battery voltage measurement with voltage divider
  // Assuming 2:1 voltage divider (battery max 4.2V -> ADC max 2.1V)
  // ADC reads 0-4095 for 0-3.3V range
  
  const int batteryPin = 1;
  const float adcMax = 4095.0;
  const float adcVoltage = 3.3;
  const float voltageDivider = 2.0;
  
  int rawValue = analogRead(batteryPin);
  float voltage = (rawValue / adcMax) * adcVoltage * voltageDivider;
  
  return voltage;
}
