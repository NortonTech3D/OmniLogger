/*
 * Data Logger for OmniLogger
 * Handles SD card operations and data logging
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

#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <Preferences.h>

class DataLogger {
public:
  DataLogger() : initialized(false), fileOpen(false), totalDataPoints(0), 
                 bufferEnabled(false), bufferCount(0), lastFlushTime(0), sdInitialized(false) {
    // Initialize NVS buffer
    if (bufferPrefs.begin("databuffer", false)) {
      bufferCount = bufferPrefs.getUInt("count", 0);
      Serial.printf("Restored %d buffered entries from NVS\n", bufferCount);
    }
  }
  
  bool begin(int csPin) {
    this->csPin = csPin;
    lastFlushTime = millis();
    
    // Don't initialize SD card here - lazy init when needed
    Serial.println("DataLogger initialized (SD card will be initialized on first flush)");
    initialized = true;
    
    return true;
  }
  
  bool initSDCard() {
    if (sdInitialized) {
      return true;
    }
    
    Serial.println("Initializing SD card...");
    
    if (!SD.begin(csPin)) {
      Serial.println("SD card initialization failed!");
      sdInitialized = false;
      return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      Serial.println("No SD card attached!");
      sdInitialized = false;
      return false;
    }
    
    Serial.print("SD Card Type: ");
    switch (cardType) {
      case CARD_MMC:
        Serial.println("MMC");
        break;
      case CARD_SD:
        Serial.println("SDSC");
        break;
      case CARD_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    sdInitialized = true;
    
    // Count existing data points
    countDataPoints();
    
    return true;
  }
  
  bool logData(const String& data) {
    if (!initialized && !bufferEnabled) {
      Serial.println("DataLogger not initialized and buffering disabled!");
      return false;
    }
    
    // If buffering is enabled, add to NVS buffer
    if (bufferEnabled) {
      if (bufferCount < MAX_BUFFER_SIZE) {
        // Store data point in NVS
        char key[16];
        snprintf(key, sizeof(key), "d%d", bufferCount);
        bufferPrefs.putString(key, data);
        bufferCount++;
        bufferPrefs.putUInt("count", bufferCount);
        
        Serial.printf("Data buffered to NVS (count: %d/%d)\n", bufferCount, MAX_BUFFER_SIZE);
        
        // Check if buffer threshold reached (80% full)
        if (bufferCount >= (MAX_BUFFER_SIZE * 80 / 100)) {
          Serial.println("Buffer 80% full, triggering flush...");
          flushBuffer();
        }
        
        return true;
      } else {
        // Buffer full, force flush then add current data
        Serial.println("Buffer full, forcing flush...");
        flushBuffer();
        
        // Store current data point
        char key[16];
        snprintf(key, sizeof(key), "d%d", bufferCount);
        bufferPrefs.putString(key, data);
        bufferCount++;
        bufferPrefs.putUInt("count", bufferCount);
        
        return true;
      }
    }
    
    // Direct write to SD card (original behavior)
    return writeToSD(data);
  }
  
  bool writeToSD(const String& data) {
    // Lazy init SD card on first write
    if (!sdInitialized && !initSDCard()) {
      Serial.println("SD card not initialized and init failed!");
      return false;
    }
    
    // Get current date for filename
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char filename[32];
    snprintf(filename, sizeof(filename), "/data_%04d%02d%02d.csv",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    // Check if file exists, if not create with header
    bool fileExists = SD.exists(filename);
    
    File dataFile = SD.open(filename, FILE_APPEND);
    if (!dataFile) {
      Serial.printf("Failed to open file: %s\n", filename);
      return false;
    }
    
    // Write header if new file
    if (!fileExists) {
      // Note: Header should be written by the caller based on sensor config
      // For now, we'll skip the header as it's dynamic
    }
    
    // Write data
    dataFile.println(data);
    dataFile.close();
    
    totalDataPoints++;
    
    return true;
  }
  
  bool writeHeader(const String& header) {
    // Lazy init SD card
    if (!sdInitialized && !initSDCard()) {
      return false;
    }
    
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char filename[32];
    snprintf(filename, sizeof(filename), "/data_%04d%02d%02d.csv",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    // Only write header if file doesn't exist
    if (SD.exists(filename)) {
      return true;  // File exists, don't overwrite
    }
    
    File dataFile = SD.open(filename, FILE_WRITE);
    if (!dataFile) {
      return false;
    }
    
    dataFile.println(header);
    dataFile.close();
    
    return true;
  }
  
  void flush() {
    if (bufferEnabled && bufferCount > 0) {
      flushBuffer();
    }
    // SD library auto-flushes on close
  }
  
  void setBufferingEnabled(bool enabled) {
    bufferEnabled = enabled;
    Serial.printf("Data buffering %s\n", enabled ? "enabled" : "disabled");
  }
  
  bool flushBuffer() {
    // Lazy init SD card on flush
    if (!sdInitialized && !initSDCard()) {
      Serial.println("Cannot flush buffer - SD card init failed!");
      return false;
    }
    
    if (bufferCount == 0) {
      return true;  // Nothing to flush
    }
    
    Serial.printf("Flushing %d buffered data points from NVS to SD card...\n", bufferCount);
    
    int successCount = 0;
    int totalCount = bufferCount;
    
    // Read buffered data from NVS and write to SD
    for (int i = 0; i < totalCount; i++) {
      char key[16];
      snprintf(key, sizeof(key), "d%d", i);
      String data = bufferPrefs.getString(key, "");
      
      if (data.length() > 0) {
        if (writeToSD(data)) {
          successCount++;
        }
      }
    }
    
    // Clear NVS buffer
    for (int i = 0; i < totalCount; i++) {
      char key[16];
      snprintf(key, sizeof(key), "d%d", i);
      bufferPrefs.remove(key);
    }
    bufferCount = 0;
    bufferPrefs.putUInt("count", 0);
    lastFlushTime = millis();
    
    Serial.printf("Flushed %d/%d data points successfully\n", successCount, totalCount);
    return successCount > 0;
  }
  
  bool shouldFlush(unsigned long flushIntervalMs) {
    if (!bufferEnabled || bufferCount == 0) {
      return false;
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsed;
    
    // Handle millis() rollover (occurs every ~49.7 days)
    if (currentTime >= lastFlushTime) {
      elapsed = currentTime - lastFlushTime;
    } else {
      // Rollover occurred
      elapsed = (0xFFFFFFFF - lastFlushTime) + currentTime + 1;
    }
    
    return elapsed >= flushIntervalMs;
  }
  
  int getBufferCount() const {
    return bufferCount;
  }
  
  int getBufferCapacity() const {
    return MAX_BUFFER_SIZE;
  }
  
  uint64_t getTotalSize() const {
    if (!sdInitialized) return 0;
    return SD.cardSize();
  }
  
  uint64_t getUsedSize() const {
    if (!sdInitialized) return 0;
    return SD.usedBytes();
  }
  
  uint64_t getFreeSize() const {
    if (!sdInitialized) return 0;
    return SD.totalBytes() - SD.usedBytes();
  }
  
  uint32_t getDataPointCount() const {
    return totalDataPoints;
  }
  
  bool isHealthy() const {
    if (!sdInitialized) return false;
    
    // Basic health check - can we write a test file?
    File testFile = SD.open("/health_check.tmp", FILE_WRITE);
    if (!testFile) {
      return false;
    }
    testFile.println("OK");
    testFile.close();
    SD.remove("/health_check.tmp");
    
    return true;
  }
  
  String getCardInfo() const {
    if (!sdInitialized) return "Not initialized";
    
    String info = "";
    uint8_t cardType = SD.cardType();
    
    info += "Type: ";
    switch (cardType) {
      case CARD_MMC:
        info += "MMC";
        break;
      case CARD_SD:
        info += "SDSC";
        break;
      case CARD_SDHC:
        info += "SDHC";
        break;
      default:
        info += "UNKNOWN";
    }
    
    info += ", Size: " + String(SD.cardSize() / (1024 * 1024)) + "MB";
    info += ", Used: " + String(SD.usedBytes() / (1024 * 1024)) + "MB";
    
    return info;
  }
  
  bool listFiles(String& output, const char* dirname = "/") {
    if (!sdInitialized) return false;
    
    File root = SD.open(dirname);
    if (!root) {
      return false;
    }
    
    if (!root.isDirectory()) {
      root.close();
      return false;
    }
    
    output = "";
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        output += String(file.name()) + " (" + String(file.size()) + " bytes)\n";
      }
      file = root.openNextFile();
    }
    
    root.close();
    return true;
  }
  
  bool downloadFile(const char* filename, String& content) {
    if (!sdInitialized) return false;
    
    File file = SD.open(filename);
    if (!file) {
      return false;
    }
    
    content = "";
    while (file.available()) {
      content += (char)file.read();
    }
    
    file.close();
    return true;
  }
  
  bool streamFile(const char* filename, WebServer& server) {
    if (!sdInitialized) return false;
    
    File file = SD.open(filename);
    if (!file) {
      return false;
    }
    
    size_t sent = server.streamFile(file, "text/csv");
    file.close();
    return sent > 0;
  }

private:
  bool initialized;
  bool fileOpen;
  int csPin;
  uint32_t totalDataPoints;
  bool sdInitialized;
  
  // Data buffering with NVS persistence
  static const int MAX_BUFFER_SIZE = 100;  // Buffer up to 100 data points
  Preferences bufferPrefs;  // NVS storage for buffer
  int bufferCount;
  bool bufferEnabled;
  unsigned long lastFlushTime;
  
  void countDataPoints() {
    totalDataPoints = 0;
    
    File root = SD.open("/");
    if (!root) return;
    
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        if (filename.startsWith("/data_") && filename.endsWith(".csv")) {
          // Store filename before any file handle changes
          String fullPath = filename;
          file.close();  // Close the directory entry file handle first
          
          // Now safely open the data file using the stored path
          File dataFile = SD.open(fullPath);
          if (dataFile) {
            bool firstLine = true;
            while (dataFile.available()) {
              String line = dataFile.readStringUntil('\n');
              if (!firstLine && line.length() > 0) {
                totalDataPoints++;
              }
              firstLine = false;
            }
            dataFile.close();
          }
          
          // Continue to next file
          file = root.openNextFile();
          continue;
        }
      }
      file.close();
      file = root.openNextFile();
    }
    
    root.close();
  }
};

#endif // DATALOGGER_H
