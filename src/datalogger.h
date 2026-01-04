/*
 * Data Logger for OmniLogger
 * Handles SD card operations and data logging
 */

#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

class DataLogger {
public:
  DataLogger() : initialized(false), fileOpen(false), totalDataPoints(0) {}
  
  bool begin(int csPin) {
    this->csPin = csPin;
    
    Serial.println("Initializing SD card...");
    
    if (!SD.begin(csPin)) {
      Serial.println("SD card initialization failed!");
      initialized = false;
      return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      Serial.println("No SD card attached!");
      initialized = false;
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
    
    initialized = true;
    
    // Count existing data points
    countDataPoints();
    
    return true;
  }
  
  bool logData(const String& data) {
    if (!initialized) {
      Serial.println("SD card not initialized!");
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
    if (!initialized) return false;
    
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
    // SD library auto-flushes on close
  }
  
  uint64_t getTotalSize() const {
    if (!initialized) return 0;
    return SD.cardSize();
  }
  
  uint64_t getUsedSize() const {
    if (!initialized) return 0;
    return SD.usedBytes();
  }
  
  uint64_t getFreeSize() const {
    if (!initialized) return 0;
    return SD.totalBytes() - SD.usedBytes();
  }
  
  uint32_t getDataPointCount() const {
    return totalDataPoints;
  }
  
  bool isHealthy() const {
    if (!initialized) return false;
    
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
    if (!initialized) return "Not initialized";
    
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
    if (!initialized) return false;
    
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
    if (!initialized) return false;
    
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

private:
  bool initialized;
  bool fileOpen;
  int csPin;
  uint32_t totalDataPoints;
  
  void countDataPoints() {
    totalDataPoints = 0;
    
    File root = SD.open("/");
    if (!root) return;
    
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        if (filename.startsWith("/data_") && filename.endsWith(".csv")) {
          // Count lines in file (excluding header)
          File dataFile = SD.open(file.name());
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
        }
      }
      file = root.openNextFile();
    }
    
    root.close();
  }
};

#endif // DATALOGGER_H
