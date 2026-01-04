/*
 * Configuration Management for OmniLogger
 * Handles storing and loading device configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <algorithm>

// Pin definitions for ESP32-S2 Mini
#define DEFAULT_SD_CS 12
#define DEFAULT_I2C_SDA 33
#define DEFAULT_I2C_SCL 35

// Sensor types
enum SensorType {
  SENSOR_NONE = 0,
  SENSOR_BME280 = 1,
  SENSOR_DHT22 = 2,
  SENSOR_DS18B20 = 3,
  SENSOR_ANALOG = 4
};

struct SensorConfig {
  SensorType type;
  int pin;           // For digital sensors (DHT, DS18B20) or analog pin
  char name[32];     // Custom sensor name
  bool enabled;
};

class Config {
public:
  // WiFi settings
  char wifiSSID[64];
  char wifiPassword[64];
  char apSSID[64];
  char apPassword[64];
  
  // Time settings
  int timezoneOffset;  // Hours offset from UTC
  
  // Measurement settings
  unsigned int measurementInterval;  // Seconds between measurements
  bool deepSleepEnabled;
  
  // Data buffering settings
  bool bufferingEnabled;
  unsigned int flushInterval;  // Seconds between buffer flushes to SD card
  
  // Pin configuration
  int sdCardCS;
  int i2cSDA;
  int i2cSCL;
  int batteryPin;
  
  // Sensor configuration (support up to 8 sensors)
  static const int MAX_SENSORS = 8;
  SensorConfig sensors[MAX_SENSORS];
  
  Config() {
    // Set defaults
    strcpy(wifiSSID, "");
    strcpy(wifiPassword, "");
    strcpy(apSSID, "OmniLogger");
    strcpy(apPassword, "omnilogger123");
    
    timezoneOffset = 0;
    measurementInterval = 60;  // Default 60 seconds
    deepSleepEnabled = false;
    
    bufferingEnabled = false;
    flushInterval = 300;  // Default 5 minutes
    
    sdCardCS = DEFAULT_SD_CS;
    i2cSDA = DEFAULT_I2C_SDA;
    i2cSCL = DEFAULT_I2C_SCL;
    batteryPin = 1;
    
    // Initialize sensors
    for (int i = 0; i < MAX_SENSORS; i++) {
      sensors[i].type = SENSOR_NONE;
      sensors[i].pin = -1;
      sensors[i].enabled = false;
      snprintf(sensors[i].name, sizeof(sensors[i].name), "Sensor%d", i + 1);
    }
    
    // Default sensor setup - BME280 on I2C
    sensors[0].type = SENSOR_BME280;
    sensors[0].pin = 0;  // I2C address index
    sensors[0].enabled = true;
    strncpy(sensors[0].name, "Environment", sizeof(sensors[0].name) - 1);
    sensors[0].name[sizeof(sensors[0].name) - 1] = '\0';
  }
  
  bool begin() {
    return prefs.begin("omnilogger", false);
  }
  
  void load() {
    // Load WiFi settings
    prefs.getString("wifiSSID", wifiSSID, sizeof(wifiSSID));
    wifiSSID[sizeof(wifiSSID) - 1] = '\0';
    prefs.getString("wifiPass", wifiPassword, sizeof(wifiPassword));
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';
    prefs.getString("apSSID", apSSID, sizeof(apSSID));
    apSSID[sizeof(apSSID) - 1] = '\0';
    prefs.getString("apPass", apPassword, sizeof(apPassword));
    apPassword[sizeof(apPassword) - 1] = '\0';
    
    // Validate AP password length (WPA2 requires min 8 characters)
    if (strlen(apPassword) < 8) {
      strncpy(apPassword, "omnilogger123", sizeof(apPassword) - 1);
      apPassword[sizeof(apPassword) - 1] = '\0';
    }
    
    // Load time settings
    timezoneOffset = prefs.getInt("tzOffset", 0);
    
    // Load measurement settings
    measurementInterval = std::max(1U, prefs.getUInt("measInterval", 60));
    deepSleepEnabled = prefs.getBool("deepSleep", false);
    
    // Load buffering settings
    bufferingEnabled = prefs.getBool("bufferEn", false);
    flushInterval = std::max(1U, prefs.getUInt("flushInt", 300));
    
    // Load pin configuration
    sdCardCS = prefs.getInt("sdCS", DEFAULT_SD_CS);
    i2cSDA = prefs.getInt("i2cSDA", DEFAULT_I2C_SDA);
    i2cSCL = prefs.getInt("i2cSCL", DEFAULT_I2C_SCL);
    batteryPin = prefs.getInt("batteryPin", 1);
    
    // Load sensor configuration
    for (int i = 0; i < MAX_SENSORS; i++) {
      char key[16];
      snprintf(key, sizeof(key), "s%d_type", i);
      sensors[i].type = (SensorType)prefs.getUInt(key, sensors[i].type);
      
      snprintf(key, sizeof(key), "s%d_pin", i);
      sensors[i].pin = prefs.getInt(key, sensors[i].pin);
      
      snprintf(key, sizeof(key), "s%d_name", i);
      prefs.getString(key, sensors[i].name, sizeof(sensors[i].name));
      sensors[i].name[sizeof(sensors[i].name) - 1] = '\0';
      
      snprintf(key, sizeof(key), "s%d_en", i);
      sensors[i].enabled = prefs.getBool(key, sensors[i].enabled);
    }
  }
  
  void save() {
    // Save WiFi settings
    prefs.putString("wifiSSID", wifiSSID);
    prefs.putString("wifiPass", wifiPassword);
    prefs.putString("apSSID", apSSID);
    prefs.putString("apPass", apPassword);
    
    // Save time settings
    prefs.putInt("tzOffset", timezoneOffset);
    
    // Save measurement settings
    prefs.putUInt("measInterval", measurementInterval);
    prefs.putBool("deepSleep", deepSleepEnabled);
    
    // Save buffering settings
    prefs.putBool("bufferEn", bufferingEnabled);
    prefs.putUInt("flushInt", flushInterval);
    
    // Save pin configuration
    prefs.putInt("sdCS", sdCardCS);
    prefs.putInt("i2cSDA", i2cSDA);
    prefs.putInt("i2cSCL", i2cSCL);
    prefs.putInt("batteryPin", batteryPin);
    
    // Save sensor configuration
    for (int i = 0; i < MAX_SENSORS; i++) {
      char key[16];
      snprintf(key, sizeof(key), "s%d_type", i);
      prefs.putUInt(key, sensors[i].type);
      
      snprintf(key, sizeof(key), "s%d_pin", i);
      prefs.putInt(key, sensors[i].pin);
      
      snprintf(key, sizeof(key), "s%d_name", i);
      prefs.putString(key, sensors[i].name);
      
      snprintf(key, sizeof(key), "s%d_en", i);
      prefs.putBool(key, sensors[i].enabled);
    }
  }
  
  void reset() {
    prefs.clear();
    // Reinitialize with defaults
    Config defaults;
    *this = defaults;
    save();
  }

private:
  Preferences prefs;
};

#endif // CONFIG_H
