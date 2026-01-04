/*
 * Sensor Management for OmniLogger
 * Handles multiple sensor types and reading data
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// Sensor reading structure
struct SensorReading {
  bool valid;
  float temperature;
  float humidity;
  float pressure;
  float value;  // Generic value for analog sensors
};

class SensorManager {
public:
  SensorManager() : bme(nullptr), dht(nullptr), dallas(nullptr), oneWire(nullptr) {
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      readings[i].valid = false;
    }
  }
  
  ~SensorManager() {
    cleanup();
  }
  
  void begin(const Config& config) {
    cleanup();
    
    // Initialize I2C if needed
    bool i2cNeeded = false;
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (config.sensors[i].enabled && config.sensors[i].type == SENSOR_BME280) {
        i2cNeeded = true;
        break;
      }
    }
    
    if (i2cNeeded) {
      Wire.begin(config.i2cSDA, config.i2cSCL);
      Serial.printf("I2C initialized on SDA:%d, SCL:%d\n", config.i2cSDA, config.i2cSCL);
    }
    
    // Initialize each sensor
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (!config.sensors[i].enabled) continue;
      
      switch (config.sensors[i].type) {
        case SENSOR_BME280:
          initBME280(i, config.sensors[i]);
          break;
        case SENSOR_DHT22:
          initDHT22(i, config.sensors[i]);
          break;
        case SENSOR_DS18B20:
          initDS18B20(i, config.sensors[i]);
          break;
        case SENSOR_ANALOG:
          initAnalog(i, config.sensors[i]);
          break;
        default:
          break;
      }
    }
    
    sensorCount = 0;
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (config.sensors[i].enabled && config.sensors[i].type != SENSOR_NONE) {
        sensorCount++;
      }
    }
  }
  
  void readAllSensors() {
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (sensorTypes[i] == SENSOR_NONE) continue;
      
      readings[i].valid = false;
      
      switch (sensorTypes[i]) {
        case SENSOR_BME280:
          readBME280(i);
          break;
        case SENSOR_DHT22:
          readDHT22(i);
          break;
        case SENSOR_DS18B20:
          readDS18B20(i);
          break;
        case SENSOR_ANALOG:
          readAnalog(i);
          break;
        default:
          break;
      }
    }
  }
  
  SensorReading getReading(int index) const {
    if (index >= 0 && index < Config::MAX_SENSORS) {
      return readings[index];
    }
    SensorReading invalid = {false, 0, 0, 0, 0};
    return invalid;
  }
  
  const char* getSensorName(int index) const {
    if (index >= 0 && index < Config::MAX_SENSORS) {
      return sensorNames[index];
    }
    return "";
  }
  
  SensorType getSensorType(int index) const {
    if (index >= 0 && index < Config::MAX_SENSORS) {
      return sensorTypes[index];
    }
    return SENSOR_NONE;
  }
  
  int getSensorCount() const {
    return sensorCount;
  }
  
  void printReadings() const {
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (sensorTypes[i] == SENSOR_NONE || !readings[i].valid) continue;
      
      Serial.printf("Sensor %d (%s): ", i, sensorNames[i]);
      
      switch (sensorTypes[i]) {
        case SENSOR_BME280:
          Serial.printf("Temp=%.2f°C, Humidity=%.2f%%, Pressure=%.2fhPa\n",
                       readings[i].temperature, readings[i].humidity, readings[i].pressure);
          break;
        case SENSOR_DHT22:
          Serial.printf("Temp=%.2f°C, Humidity=%.2f%%\n",
                       readings[i].temperature, readings[i].humidity);
          break;
        case SENSOR_DS18B20:
          Serial.printf("Temp=%.2f°C\n", readings[i].temperature);
          break;
        case SENSOR_ANALOG:
          Serial.printf("Value=%.2f\n", readings[i].value);
          break;
        default:
          break;
      }
    }
  }
  
  String getCSVData(const char* timestamp) const {
    String csv = timestamp;
    
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (sensorTypes[i] == SENSOR_NONE) continue;
      
      if (!readings[i].valid) {
        switch (sensorTypes[i]) {
          case SENSOR_BME280:
            csv += ",,,";  // temp, humidity, pressure
            break;
          case SENSOR_DHT22:
            csv += ",,";  // temp, humidity
            break;
          case SENSOR_DS18B20:
            csv += ",";  // temp
            break;
          case SENSOR_ANALOG:
            csv += ",";  // value
            break;
          default:
            break;
        }
      } else {
        switch (sensorTypes[i]) {
          case SENSOR_BME280:
            csv += "," + String(readings[i].temperature, 2);
            csv += "," + String(readings[i].humidity, 2);
            csv += "," + String(readings[i].pressure, 2);
            break;
          case SENSOR_DHT22:
            csv += "," + String(readings[i].temperature, 2);
            csv += "," + String(readings[i].humidity, 2);
            break;
          case SENSOR_DS18B20:
            csv += "," + String(readings[i].temperature, 2);
            break;
          case SENSOR_ANALOG:
            csv += "," + String(readings[i].value, 2);
            break;
          default:
            break;
        }
      }
    }
    
    return csv;
  }
  
  String getCSVHeader() const {
    String header = "Timestamp";
    
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (sensorTypes[i] == SENSOR_NONE) continue;
      
      String prefix = String(sensorNames[i]);
      
      switch (sensorTypes[i]) {
        case SENSOR_BME280:
          header += "," + prefix + "_Temp_C";
          header += "," + prefix + "_Humidity_%";
          header += "," + prefix + "_Pressure_hPa";
          break;
        case SENSOR_DHT22:
          header += "," + prefix + "_Temp_C";
          header += "," + prefix + "_Humidity_%";
          break;
        case SENSOR_DS18B20:
          header += "," + prefix + "_Temp_C";
          break;
        case SENSOR_ANALOG:
          header += "," + prefix + "_Value";
          break;
        default:
          break;
      }
    }
    
    return header;
  }

private:
  Adafruit_BME280* bme;
  DHT* dht;
  DallasTemperature* dallas;
  OneWire* oneWire;
  
  SensorReading readings[Config::MAX_SENSORS];
  SensorType sensorTypes[Config::MAX_SENSORS] = {SENSOR_NONE};
  char sensorNames[Config::MAX_SENSORS][32];
  int sensorCount;
  
  void cleanup() {
    if (bme) {
      delete bme;
      bme = nullptr;
    }
    if (dht) {
      delete dht;
      dht = nullptr;
    }
    if (dallas) {
      delete dallas;
      dallas = nullptr;
    }
    if (oneWire) {
      delete oneWire;
      oneWire = nullptr;
    }
  }
  
  void initBME280(int index, const SensorConfig& config) {
    Serial.printf("Initializing BME280 sensor %d...\n", index);
    
    if (!bme) {
      bme = new Adafruit_BME280();
    }
    
    if (bme->begin(0x76)) {
      sensorTypes[index] = SENSOR_BME280;
      strcpy(sensorNames[index], config.name);
      Serial.println("BME280 initialized successfully");
    } else if (bme->begin(0x77)) {
      sensorTypes[index] = SENSOR_BME280;
      strcpy(sensorNames[index], config.name);
      Serial.println("BME280 initialized successfully (alt address)");
    } else {
      Serial.println("Failed to initialize BME280");
      sensorTypes[index] = SENSOR_NONE;
    }
  }
  
  void initDHT22(int index, const SensorConfig& config) {
    Serial.printf("Initializing DHT22 sensor %d on pin %d...\n", index, config.pin);
    
    dht = new DHT(config.pin, DHT22);
    dht->begin();
    
    sensorTypes[index] = SENSOR_DHT22;
    strcpy(sensorNames[index], config.name);
  }
  
  void initDS18B20(int index, const SensorConfig& config) {
    Serial.printf("Initializing DS18B20 sensor %d on pin %d...\n", index, config.pin);
    
    oneWire = new OneWire(config.pin);
    dallas = new DallasTemperature(oneWire);
    dallas->begin();
    
    sensorTypes[index] = SENSOR_DS18B20;
    strcpy(sensorNames[index], config.name);
  }
  
  void initAnalog(int index, const SensorConfig& config) {
    Serial.printf("Initializing analog sensor %d on pin %d...\n", index, config.pin);
    
    pinMode(config.pin, INPUT);
    sensorTypes[index] = SENSOR_ANALOG;
    strcpy(sensorNames[index], config.name);
  }
  
  void readBME280(int index) {
    if (!bme) return;
    
    readings[index].temperature = bme->readTemperature();
    readings[index].humidity = bme->readHumidity();
    readings[index].pressure = bme->readPressure() / 100.0F;
    readings[index].valid = true;
  }
  
  void readDHT22(int index) {
    if (!dht) return;
    
    float temp = dht->readTemperature();
    float hum = dht->readHumidity();
    
    if (!isnan(temp) && !isnan(hum)) {
      readings[index].temperature = temp;
      readings[index].humidity = hum;
      readings[index].valid = true;
    }
  }
  
  void readDS18B20(int index) {
    if (!dallas) return;
    
    dallas->requestTemperatures();
    float temp = dallas->getTempCByIndex(0);
    
    if (temp != DEVICE_DISCONNECTED_C) {
      readings[index].temperature = temp;
      readings[index].valid = true;
    }
  }
  
  void readAnalog(int index) {
    int rawValue = analogRead(sensorTypes[index]);
    readings[index].value = rawValue * (3.3 / 4095.0);
    readings[index].valid = true;
  }
};

#endif // SENSORS_H
