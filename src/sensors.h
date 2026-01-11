/*
 * Sensor Management for OmniLogger
 * Handles multiple sensor types and reading data
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
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <cmath>
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
  SensorManager() : sensorCount(0), i2cInitialized(false) {
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      readings[i].valid = false;
      dhtSensors[i] = nullptr;
      dallasSensors[i] = nullptr;
      oneWireSensors[i] = nullptr;
      bmeSensors[i] = nullptr;
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
      if (!i2cInitialized) {
        Wire.begin(config.i2cSDA, config.i2cSCL);
        i2cInitialized = true;
        Serial.printf("I2C initialized on SDA:%d, SCL:%d\n", config.i2cSDA, config.i2cSCL);
      }
      // Try to recover I2C bus if stuck (clock out any stuck slaves)
      recoverI2CBus(config.i2cSDA, config.i2cSCL);
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
    // Pre-allocate string buffer to reduce memory fragmentation
    String csv;
    csv.reserve(256);  // Reserve reasonable size upfront
    csv = timestamp;
    
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
        char buffer[32];
        switch (sensorTypes[i]) {
          case SENSOR_BME280:
            snprintf(buffer, sizeof(buffer), ",%.2f,%.2f,%.2f", 
                    readings[i].temperature, readings[i].humidity, readings[i].pressure);
            csv += buffer;
            break;
          case SENSOR_DHT22:
            snprintf(buffer, sizeof(buffer), ",%.2f,%.2f", 
                    readings[i].temperature, readings[i].humidity);
            csv += buffer;
            break;
          case SENSOR_DS18B20:
            snprintf(buffer, sizeof(buffer), ",%.2f", readings[i].temperature);
            csv += buffer;
            break;
          case SENSOR_ANALOG:
            snprintf(buffer, sizeof(buffer), ",%.2f", readings[i].value);
            csv += buffer;
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
  Adafruit_BME280* bmeSensors[Config::MAX_SENSORS];
  DHT* dhtSensors[Config::MAX_SENSORS];
  DallasTemperature* dallasSensors[Config::MAX_SENSORS];
  OneWire* oneWireSensors[Config::MAX_SENSORS];
  
  SensorReading readings[Config::MAX_SENSORS];
  SensorType sensorTypes[Config::MAX_SENSORS] = {SENSOR_NONE};
  char sensorNames[Config::MAX_SENSORS][32];
  int sensorPins[Config::MAX_SENSORS] = {-1};
  int sensorCount;
  bool i2cInitialized = false;
  
  void cleanup() {
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      if (bmeSensors[i]) {
        delete bmeSensors[i];
        bmeSensors[i] = nullptr;
      }
      if (dhtSensors[i]) {
        delete dhtSensors[i];
        dhtSensors[i] = nullptr;
      }
      if (dallasSensors[i]) {
        delete dallasSensors[i];
        dallasSensors[i] = nullptr;
      }
      if (oneWireSensors[i]) {
        delete oneWireSensors[i];
        oneWireSensors[i] = nullptr;
      }
      // Reset sensor type to avoid stale readings
      sensorTypes[i] = SENSOR_NONE;
      sensorPins[i] = -1;
    }
  }
  
  // I2C bus recovery - clocks out stuck slaves by toggling SCL
  void recoverI2CBus(int sdaPin, int sclPin) {
    Wire.end();
    
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, OUTPUT);
    
    // Clock out up to 9 bits to release stuck slave
    for (int i = 0; i < 9; i++) {
      digitalWrite(sclPin, LOW);
      delayMicroseconds(5);
      digitalWrite(sclPin, HIGH);
      delayMicroseconds(5);
      
      // Check if SDA is released
      if (digitalRead(sdaPin) == HIGH) {
        break;
      }
    }
    
    // Generate STOP condition
    pinMode(sdaPin, OUTPUT);
    digitalWrite(sdaPin, LOW);
    delayMicroseconds(5);
    digitalWrite(sclPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sdaPin, HIGH);
    delayMicroseconds(5);
    
    // Reinitialize I2C
    Wire.begin(sdaPin, sclPin);
  }
  
  void initBME280(int index, const SensorConfig& config) {
    Serial.printf("Initializing BME280 sensor %d...\n", index);
    
    bmeSensors[index] = new Adafruit_BME280();
    
    // Use config.pin to determine address: 0 = 0x76, 1 = 0x77
    uint8_t addr = (config.pin == 1) ? 0x77 : 0x76;
    
    if (bmeSensors[index]->begin(addr)) {
      sensorTypes[index] = SENSOR_BME280;
      strncpy(sensorNames[index], config.name, sizeof(sensorNames[index]) - 1);
      sensorNames[index][sizeof(sensorNames[index]) - 1] = '\0';
      sensorPins[index] = config.pin;
      Serial.printf("BME280 initialized successfully at address 0x%02X\n", addr);
    } else {
      delete bmeSensors[index];
      bmeSensors[index] = nullptr;
      Serial.printf("Failed to initialize BME280 at address 0x%02X\n", addr);
      sensorTypes[index] = SENSOR_NONE;
    }
  }
  
  void initDHT22(int index, const SensorConfig& config) {
    Serial.printf("Initializing DHT22 sensor %d on pin %d...\n", index, config.pin);
    
    dhtSensors[index] = new DHT(config.pin, DHT22);
    dhtSensors[index]->begin();
    
    sensorTypes[index] = SENSOR_DHT22;
    strncpy(sensorNames[index], config.name, sizeof(sensorNames[index]) - 1);
    sensorNames[index][sizeof(sensorNames[index]) - 1] = '\0';
    sensorPins[index] = config.pin;
  }
  
  void initDS18B20(int index, const SensorConfig& config) {
    Serial.printf("Initializing DS18B20 sensor %d on pin %d...\n", index, config.pin);
    
    oneWireSensors[index] = new OneWire(config.pin);
    dallasSensors[index] = new DallasTemperature(oneWireSensors[index]);
    dallasSensors[index]->begin();
    
    // Set to 10-bit resolution for faster reads (0.25°C precision, ~187ms vs 750ms for 12-bit)
    dallasSensors[index]->setResolution(10);
    // Enable async/non-blocking mode
    dallasSensors[index]->setWaitForConversion(false);
    
    sensorTypes[index] = SENSOR_DS18B20;
    strncpy(sensorNames[index], config.name, sizeof(sensorNames[index]) - 1);
    sensorNames[index][sizeof(sensorNames[index]) - 1] = '\0';
    sensorPins[index] = config.pin;
  }
  
  void initAnalog(int index, const SensorConfig& config) {
    Serial.printf("Initializing analog sensor %d on pin %d...\n", index, config.pin);
    
    pinMode(config.pin, INPUT);
    sensorTypes[index] = SENSOR_ANALOG;
    strncpy(sensorNames[index], config.name, sizeof(sensorNames[index]) - 1);
    sensorNames[index][sizeof(sensorNames[index]) - 1] = '\0';
    sensorPins[index] = config.pin;
  }
  
  void readBME280(int index) {
    if (index < 0 || index >= Config::MAX_SENSORS || !bmeSensors[index]) return;
    
    float temp = bmeSensors[index]->readTemperature();
    float hum = bmeSensors[index]->readHumidity();
    float pres = bmeSensors[index]->readPressure() / 100.0F;
    
    // Validate readings are within reasonable ranges
    if (!isnan(temp) && !isnan(hum) && !isnan(pres) &&
        temp >= -40.0 && temp <= 85.0 &&      // BME280 operating range
        hum >= 0.0 && hum <= 100.0 &&         // Valid humidity range
        pres >= 300.0 && pres <= 1100.0) {    // Valid pressure range
      readings[index].temperature = temp;
      readings[index].humidity = hum;
      readings[index].pressure = pres;
      readings[index].valid = true;
    } else {
      readings[index].valid = false;
      Serial.printf("BME280 sensor %d: Invalid readings detected\n", index);
    }
  }
  
  void readDHT22(int index) {
    if (index < 0 || index >= Config::MAX_SENSORS || !dhtSensors[index]) return;
    
    float temp = dhtSensors[index]->readTemperature();
    float hum = dhtSensors[index]->readHumidity();
    
    // Validate readings are within DHT22 specs
    if (!isnan(temp) && !isnan(hum) &&
        temp >= -40.0 && temp <= 80.0 &&      // DHT22 operating range
        hum >= 0.0 && hum <= 100.0) {         // Valid humidity range
      readings[index].temperature = temp;
      readings[index].humidity = hum;
      readings[index].valid = true;
    } else {
      readings[index].valid = false;
      Serial.printf("DHT22 sensor %d: Invalid readings detected\n", index);
    }
  }
  
  void readDS18B20(int index) {
    if (index < 0 || index >= Config::MAX_SENSORS || !dallasSensors[index]) return;
    
    dallasSensors[index]->requestTemperatures();
    
    // Wait for conversion with timeout (10-bit = ~187ms, add margin)
    unsigned long startTime = millis();
    while (!dallasSensors[index]->isConversionComplete()) {
      if (millis() - startTime > 300) {  // 300ms timeout
        readings[index].valid = false;
        Serial.printf("DS18B20 sensor %d: Conversion timeout\n", index);
        return;
      }
      delay(10);  // Short delay to avoid busy-waiting
    }
    
    float temp = dallasSensors[index]->getTempCByIndex(0);
    
    // Validate temperature is within DS18B20 range
    if (temp != DEVICE_DISCONNECTED_C && 
        temp >= -55.0 && temp <= 125.0) {     // DS18B20 operating range
      readings[index].temperature = temp;
      readings[index].valid = true;
    } else {
      readings[index].valid = false;
      if (temp == DEVICE_DISCONNECTED_C) {
        Serial.printf("DS18B20 sensor %d: Device disconnected\n", index);
      } else {
        Serial.printf("DS18B20 sensor %d: Invalid reading\n", index);
      }
    }
  }
  
  void readAnalog(int index) {
    if (index < 0 || index >= Config::MAX_SENSORS || sensorPins[index] < 0) return;
    
    // Multisampling for noise reduction
    const int numSamples = 8;
    uint32_t sum = 0;
    for (int i = 0; i < numSamples; i++) {
      sum += analogRead(sensorPins[index]);
      delayMicroseconds(50);
    }
    int rawValue = sum / numSamples;
    
    // ESP32-S2 ADC is 13-bit (0-8191), but analogRead returns 12-bit by default
    // Using default 12-bit for compatibility
    if (rawValue >= 0 && rawValue <= 4095) {
      readings[index].value = rawValue * (3.3 / 4095.0);
      readings[index].valid = true;
    } else {
      readings[index].valid = false;
      Serial.printf("Analog sensor %d: Invalid ADC reading\n", index);
    }
  }
};

#endif // SENSORS_H
