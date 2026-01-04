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
  DataLogger() : initialized(false), fileOpen(false), totalDataPoints(0), 
                 bufferEnabled(false), bufferCount(0), lastFlushTime(0) {
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
      buffer[i] = "";
    }
  }
  
  bool begin(int csPin) {
    this->csPin = csPin;
    lastFlushTime = millis();
    
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
    if (!initialized && !bufferEnabled) {
      Serial.println("SD card not initialized and buffering disabled!");
      return false;
    }
    
    // If buffering is enabled, add to buffer
    if (bufferEnabled) {
      if (bufferCount < MAX_BUFFER_SIZE) {
        buffer[bufferCount++] = data;
        Serial.printf("Data buffered (count: %d/%d)\n", bufferCount, MAX_BUFFER_SIZE);
        return true;
      } else {
        // Buffer full, force flush then add current data
        Serial.println("Buffer full, forcing flush...");
        flushBuffer();
        buffer[bufferCount++] = data;
        return true;
      }
    }
    
    // Direct write to SD card (original behavior)
    return writeToSD(data);
  }
  
  bool writeToSD(const String& data) {
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
    if (!initialized) {
      Serial.println("Cannot flush buffer - SD card not initialized!");
      return false;
    }
    
    if (bufferCount == 0) {
      return true;  // Nothing to flush
    }
    
    Serial.printf("Flushing %d buffered data points to SD card...\n", bufferCount);
    
    int successCount = 0;
    int totalCount = bufferCount;
    for (int i = 0; i < totalCount; i++) {
      if (writeToSD(buffer[i])) {
        successCount++;
      }
    }
    
    // Clear buffer
    bufferCount = 0;
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
  
  // Data buffering
  static const int MAX_BUFFER_SIZE = 100;  // Buffer up to 100 data points
  String buffer[MAX_BUFFER_SIZE];
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
