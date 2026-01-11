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
#include <sys/time.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <esp_task_wdt.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_adc_cal.h>
#include <driver/adc.h>
#include <soc/soc_caps.h>
#include "config.h"
#include "sensors.h"
#include "web_interface.h"
#include "datalogger.h"

// Watchdog timeout in seconds
#define WDT_TIMEOUT_SEC 30

// ADC calibration characteristics (stored after calibration)
static esp_adc_cal_characteristics_t* adc_chars = nullptr;
static bool adcCalibrated = false;

// RTC memory survives deep sleep - use for persistent counters
RTC_DATA_ATTR uint32_t rtcMeasurementCount = 0;
RTC_DATA_ATTR uint32_t rtcBootCount = 0;
RTC_DATA_ATTR bool rtcTimeInitialized = false;
RTC_DATA_ATTR time_t rtcLastTimestamp = 0;
RTC_DATA_ATTR uint8_t rtcErrorCount = 0;  // Track errors across reboots

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
volatile unsigned long lastButtonPress = 0;  // ISR debounce
const unsigned long DEBOUNCE_MS = 250;  // 250ms debounce

// Error tracking for reliability
uint32_t sensorErrors = 0;
uint32_t sdErrors = 0;
uint32_t wifiErrors = 0;
uint32_t consecutiveErrors = 0;
const uint32_t MAX_CONSECUTIVE_ERRORS = 5;  // Reset if too many errors

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
void checkSystemHealth();
uint32_t getFreeHeap();
void logSystemStatus();

// Get free heap memory
uint32_t getFreeHeap() {
  return ESP.getFreeHeap();
}

// Log system status for diagnostics
void logSystemStatus() {
  Serial.printf("[STATUS] Internal SRAM - Free: %u bytes, Min: %u bytes\n", 
                ESP.getFreeHeap(), ESP.getMinFreeHeap());
  if (psramFound()) {
    Serial.printf("[STATUS] PSRAM - Free: %u bytes, Total: %u bytes\n", 
                  ESP.getFreePsram(), ESP.getPsramSize());
    // Also show PSRAM heap allocation stats
    size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psramLargest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    Serial.printf("[STATUS] PSRAM heap - Free: %u bytes, Largest block: %u bytes\n",
                  psramFree, psramLargest);
  }
  Serial.printf("[STATUS] CPU: %d MHz, ADC calibrated: %s\n",
                getCpuFrequencyMhz(), adcCalibrated ? "Yes" : "No");
  Serial.printf("[STATUS] Errors - Sensor: %u, SD: %u, WiFi: %u\n",
                sensorErrors, sdErrors, wifiErrors);
}

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
  
  // === HARDWARE DIAGNOSTICS ===
  // Print chip information first
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.printf("\nChip: ESP32-S2 with %d CPU core(s)\n", chip_info.cores);
  Serial.printf("Silicon revision: %d\n", chip_info.revision);
  Serial.printf("Features: WiFi%s%s\n",
                (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "/Embedded-Flash" : "");
  
  // Print flash size
  uint32_t flash_size;
  if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
    Serial.printf("Flash size: %u MB (%s)\n", 
                  flash_size / (1024 * 1024),
                  (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
  }
  
  // === PSRAM INITIALIZATION (Must happen early, before large allocations) ===
  if (psramFound()) {
    size_t psramSize = ESP.getPsramSize();
    size_t freePsram = ESP.getFreePsram();
    Serial.printf("\nPSRAM Status:\n");
    Serial.printf("  Total: %u bytes (%.2f MB)\n", psramSize, psramSize / (1024.0 * 1024.0));
    Serial.printf("  Free:  %u bytes (%.2f MB)\n", freePsram, freePsram / (1024.0 * 1024.0));
    
    // Verify PSRAM is functional with a test allocation
    void* testAlloc = ps_malloc(1024);
    if (testAlloc) {
      Serial.println("  PSRAM allocation test: PASSED");
      free(testAlloc);
    } else {
      Serial.println("  WARNING: PSRAM allocation test FAILED!");
    }
    
    // Check if heap_caps is using PSRAM
    size_t psramHeap = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    Serial.printf("  PSRAM heap available: %u bytes\n", psramHeap);
  } else {
    Serial.println("\nWARNING: PSRAM not found!");
    Serial.println("Large operations may fail or cause memory issues.");
    Serial.println("Check hardware - ESP32-S2FN4R2 should have 2MB PSRAM.");
  }
  
  // === ADC CALIBRATION (ESP32-S2 specific) ===
  Serial.println("\nInitializing ADC calibration...");
  adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  if (adc_chars) {
    // Configure ADC1 for battery voltage reading
    adc1_config_width(ADC_WIDTH_BIT_13);  // ESP32-S2 supports 13-bit ADC
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);  // GPIO1, 0-3.3V range
    
    // Characterize ADC using eFuse calibration data
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_13, 1100, adc_chars);
    
    switch (cal_type) {
      case ESP_ADC_CAL_VAL_EFUSE_TP:
        Serial.println("  ADC calibration: Two Point (eFuse)");
        adcCalibrated = true;
        break;
      case ESP_ADC_CAL_VAL_EFUSE_VREF:
        Serial.println("  ADC calibration: Vref (eFuse)");
        adcCalibrated = true;
        break;
      case ESP_ADC_CAL_VAL_DEFAULT_VREF:
        Serial.println("  ADC calibration: Default Vref (less accurate)");
        adcCalibrated = true;
        break;
      default:
        Serial.println("  WARNING: ADC calibration failed!");
        adcCalibrated = false;
    }
  } else {
    Serial.println("  ERROR: Failed to allocate ADC calibration structure!");
  }
  
  // Print internal SRAM status
  Serial.printf("\nInternal SRAM:\n");
  Serial.printf("  Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("  Min free heap: %u bytes\n", ESP.getMinFreeHeap());
  Serial.printf("  Max alloc heap: %u bytes\n", ESP.getMaxAllocHeap());
  
  Serial.println("");
  
  bootTime = millis();
  rtcBootCount++;
  
  // Check wake reason - optimize startup for deep sleep wake
  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  bool isDeepSleepWake = (wakeReason == ESP_SLEEP_WAKEUP_TIMER);
  
  if (isDeepSleepWake) {
    Serial.println("Woke from deep sleep (timer)");
    // Use RTC values - faster than loading from NVS
    measurementCount = rtcMeasurementCount;
    timeInitialized = rtcTimeInitialized;
    
    // Restore approximate time from RTC memory
    // This adds the sleep duration to the last saved timestamp
    if (rtcTimeInitialized && rtcLastTimestamp > 0) {
      struct timeval tv;
      tv.tv_sec = rtcLastTimestamp + deviceConfig.measurementInterval;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      Serial.println("Time restored from RTC memory");
    }
  } else {
    Serial.printf("Cold boot (reason: %d)\n", wakeReason);
    // Reset error counter on cold boot
    rtcErrorCount = 0;
  }
  
  Serial.printf("Boot count: %d\n", rtcBootCount);
  
  // Check for excessive errors from previous runs
  if (rtcErrorCount > 10) {
    Serial.println("WARNING: Many errors detected in previous runs!");
    Serial.println("Consider checking hardware connections.");
    rtcErrorCount = 0;  // Reset for fresh start
  }
  
  // Initialize watchdog timer - will reset ESP32 if system hangs
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);  // true = panic/reset on timeout
  esp_task_wdt_add(NULL);  // Add current task to watchdog
  Serial.printf("Watchdog timer initialized (%d sec timeout)\n", WDT_TIMEOUT_SEC);
  
  // Reduce CPU frequency to save power (80MHz is sufficient for data logging)
  setCpuFrequencyMhz(80);
  Serial.printf("CPU frequency set to %d MHz\n", getCpuFrequencyMhz());
  
  // Set ADC attenuation for battery voltage reading (0-3.3V range)
  analogSetAttenuation(ADC_11db);  // Full range 0-3.3V
  
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
  // Feed the watchdog to prevent reset
  esp_task_wdt_reset();
  
  // Check for WiFi re-enable request from button
  if (wifiReenableRequested) {
    wifiReenableRequested = false;
    enableWiFi();
  }
  
  // Periodic system health check - interval based on measurement settings
  // If deep sleep enabled: check once per wake cycle (before each measurement)
  // If deep sleep disabled: check every 5 measurements or at minimum every 5 minutes
  static unsigned long lastHealthCheck = 0;
  
  // Calculate health check interval based on settings
  unsigned long healthCheckInterval;
  if (deviceConfig.deepSleepEnabled) {
    // With deep sleep, check right before sleep (handled in takeMeasurement flow)
    // So in the loop, only check if we've been awake a while without measuring
    healthCheckInterval = deviceConfig.measurementInterval * 1000UL;
  } else {
    // Without deep sleep: check every 5 measurements or every 5 minutes, whichever is shorter
    unsigned long fiveMeasurements = deviceConfig.measurementInterval * 5 * 1000UL;
    unsigned long fiveMinutes = 300000UL;
    healthCheckInterval = min(fiveMeasurements, fiveMinutes);
    // But at minimum check every 30 seconds if interval is very short
    healthCheckInterval = max(healthCheckInterval, 30000UL);
  }
  
  // Handle millis() rollover for health check timing
  unsigned long healthElapsed;
  if (millis() >= lastHealthCheck) {
    healthElapsed = millis() - lastHealthCheck;
  } else {
    healthElapsed = (0xFFFFFFFF - lastHealthCheck) + millis() + 1;
  }
  
  if (healthElapsed > healthCheckInterval) {
    checkSystemHealth();
    lastHealthCheck = millis();
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
  unsigned long measurementElapsed;
  
  // Handle millis() rollover (occurs every ~49.7 days)
  if (currentTime >= lastMeasurement) {
    measurementElapsed = currentTime - lastMeasurement;
  } else {
    measurementElapsed = (0xFFFFFFFF - lastMeasurement) + currentTime + 1;
  }
  
  if (measurementElapsed >= deviceConfig.measurementInterval * 1000UL) {
    takeMeasurement();
    lastMeasurement = currentTime;
    
    // If deep sleep is enabled and we're battery powered, enter sleep
    if (deviceConfig.deepSleepEnabled && readBatteryVoltage() < 5.0) {
      enterDeepSleep();
    }
  }
  
  // Resync time periodically (every 12 hours)
  static unsigned long lastTimeSync = 0;
  unsigned long currentMillis = millis();
  unsigned long timeSyncElapsed;
  if (currentMillis >= lastTimeSync) {
    timeSyncElapsed = currentMillis - lastTimeSync;
  } else {
    timeSyncElapsed = (0xFFFFFFFF - lastTimeSync) + currentMillis + 1;
  }
  if (timeInitialized && wifiEnabled && timeSyncElapsed > 12 * 60 * 60 * 1000UL) {
    syncTime();
    lastTimeSync = currentMillis;
  }
  
  // Use light sleep between loop iterations when WiFi is off (saves ~10mA)
  if (!wifiEnabled && !deviceConfig.deepSleepEnabled) {
    // Light sleep for 10ms instead of delay - wakes on any interrupt
    esp_sleep_enable_timer_wakeup(10000);  // 10ms in microseconds
    esp_light_sleep_start();
  } else {
    delay(10);
  }
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
      
      // Enable WiFi modem sleep to save power while maintaining connection
      esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
      Serial.println("WiFi modem sleep enabled");
      
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
  
  // Feed watchdog at start of measurement
  esp_task_wdt_reset();
  
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
  
  // Read all sensors with error tracking
  sensorManager.readAllSensors();
  
  // Count valid readings
  int validReadings = 0;
  for (int i = 0; i < Config::MAX_SENSORS; i++) {
    if (sensorManager.getSensorType(i) != SENSOR_NONE) {
      SensorReading reading = sensorManager.getReading(i);
      if (reading.valid) {
        validReadings++;
      }
    }
  }
  
  if (validReadings == 0 && sensorManager.getSensorCount() > 0) {
    sensorErrors++;
    consecutiveErrors++;
    Serial.println("WARNING: All sensor readings failed!");
  } else {
    consecutiveErrors = 0;  // Reset on successful read
  }
  
  // Write header for new files
  dataLogger.writeHeader(sensorManager.getCSVHeader());
  
  // Log to SD card with retry logic
  String logEntry = sensorManager.getCSVData(timestamp);
  bool logSuccess = false;
  int retries = 3;
  
  while (!logSuccess && retries > 0) {
    if (dataLogger.logData(logEntry)) {
      logSuccess = true;
      measurementCount++;
      rtcMeasurementCount = measurementCount;  // Fast RTC save
      
      // Only write to NVS every 10 measurements to reduce flash wear
      if (measurementCount % 10 == 0) {
        measurementPrefs.putUInt("count", measurementCount);
      }
      consecutiveErrors = 0;
    } else {
      retries--;
      if (retries > 0) {
        Serial.printf("SD write failed, retrying... (%d attempts left)\n", retries);
        delay(100);
        esp_task_wdt_reset();
      }
    }
  }
  
  if (logSuccess) {
    Serial.printf("Data logged successfully (count: %d)\n", measurementCount);
  } else {
    sdErrors++;
    consecutiveErrors++;
    Serial.println("ERROR: Failed to log data after retries");
  }
  
  // Print to serial
  Serial.printf("Timestamp: %s\n", timestamp);
  sensorManager.printReadings();
  Serial.printf("Battery: %.2fV, Free heap: %u bytes\n", readBatteryVoltage(), ESP.getFreeHeap());
  Serial.println("--- Measurement Complete ---\n");
}

void enterDeepSleep() {
  Serial.printf("Entering deep sleep for %d seconds...\n", deviceConfig.measurementInterval);
  
  // Run abbreviated health check before sleep (skip WiFi reconnect since we're sleeping)
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 20000) {
    Serial.printf("WARNING: Low memory before sleep! Free: %u bytes\n", freeHeap);
  }
  
  // Log error summary if any errors occurred
  if (sensorErrors > 0 || sdErrors > 0 || wifiErrors > 0) {
    Serial.printf("[PRE-SLEEP STATUS] Errors - Sensor: %u, SD: %u, WiFi: %u\n",
                  sensorErrors, sdErrors, wifiErrors);
  }
  
  // Save time to RTC memory for faster wake
  time_t now;
  time(&now);
  rtcLastTimestamp = now;
  rtcTimeInitialized = timeInitialized;
  rtcMeasurementCount = measurementCount;
  
  // Sync final measurement count to NVS before sleep
  measurementPrefs.putUInt("count", measurementCount);
  
  // Do NOT flush buffer - it persists in NVS across deep sleep
  // Buffer will be flushed when threshold is reached or manually triggered
  
  // Close NVS properly to save data
  measurementPrefs.end();
  
  // Power down WiFi completely
  if (wifiEnabled) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();  // Also stop Bluetooth to save power
  }
  
  // Power down SD card
  dataLogger.powerDown();
  
  // Hold GPIO states during deep sleep to prevent floating pins
  gpio_hold_en(GPIO_NUM_0);  // WiFi button
  gpio_deep_sleep_hold_en();
  
  // Configure wake up
  esp_sleep_enable_timer_wakeup(deviceConfig.measurementInterval * 1000000ULL);
  
  // Enter deep sleep
  Serial.println("Entering deep sleep now...");
  Serial.flush();
  esp_deep_sleep_start();
}

float readBatteryVoltage() {
  // ESP32-S2 has ADC on GPIO1-10
  // Using configured GPIO for battery voltage measurement with voltage divider
  // Assuming 2:1 voltage divider (battery max 4.2V -> ADC max 2.1V)
  
  const float voltageDivider = 2.0;
  const int numSamples = 16;  // Multisampling for noise reduction
  
  // Validate battery pin is configured (ESP32-S2 ADC1 is GPIO1-10)
  if (deviceConfig.batteryPin < 1 || deviceConfig.batteryPin > 10) {
    return 0.0;
  }
  
  // Multisampling: take multiple readings and average
  uint32_t sum = 0;
  for (int i = 0; i < numSamples; i++) {
    if (adcCalibrated && adc_chars) {
      // Use calibrated reading for accurate voltage
      // Map battery pin to ADC1 channel (GPIO1 = CH0, GPIO2 = CH1, etc.)
      adc1_channel_t channel = (adc1_channel_t)(deviceConfig.batteryPin - 1);
      int raw = adc1_get_raw(channel);
      sum += esp_adc_cal_raw_to_voltage(raw, adc_chars);
    } else {
      // Fallback to uncalibrated analogRead
      sum += analogReadMilliVolts(deviceConfig.batteryPin);
    }
    delayMicroseconds(100);  // Small delay between samples
  }
  
  // Average reading in millivolts
  float millivolts = sum / numSamples;
  
  // Convert to actual voltage considering the voltage divider
  float voltage = (millivolts / 1000.0) * voltageDivider;
  
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
  // Debounce: ignore if pressed recently
  // Note: millis() may not be fully reliable in ISR, but is safe on ESP32
  // Using volatile ensures proper synchronization with main loop
  unsigned long now = millis();
  unsigned long lastPress = lastButtonPress;  // Read volatile once
  if ((now - lastPress) > DEBOUNCE_MS || now < lastPress) {  // Handle rollover
    lastButtonPress = now;
    // Set flag to re-enable WiFi in main loop (can't do WiFi operations in ISR)
    wifiReenableRequested = true;
  }
}

// System health check - monitors resources and recovers from errors
void checkSystemHealth() {
  // Feed watchdog during health check
  esp_task_wdt_reset();
  
  // Check heap memory - warn if getting low
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t minFreeHeap = ESP.getMinFreeHeap();
  
  if (freeHeap < 20000) {
    Serial.printf("WARNING: Low memory! Free: %u bytes\n", freeHeap);
    // Force garbage collection on Strings
    String().reserve(1);
  }
  
  if (minFreeHeap < 10000) {
    Serial.printf("CRITICAL: Memory fragmentation detected! Min free: %u bytes\n", minFreeHeap);
    rtcErrorCount++;
  }
  
  // Check for WiFi connection issues in STA mode
  if (wifiEnabled && strlen(deviceConfig.wifiSSID) > 0) {
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
      wifiErrors++;
      consecutiveErrors++;
      Serial.println("WiFi connection lost, attempting reconnect...");
      
      // Try to reconnect
      WiFi.disconnect();
      WiFi.begin(deviceConfig.wifiSSID, deviceConfig.wifiPassword);
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        esp_task_wdt_reset();  // Feed watchdog during reconnect
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi reconnected successfully");
        consecutiveErrors = 0;
      } else {
        Serial.println("WiFi reconnect failed, switching to AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(deviceConfig.apSSID, deviceConfig.apPassword);
      }
    }
  }
  
  // Check for too many consecutive errors - may indicate hardware issue
  if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
    Serial.println("ERROR: Too many consecutive errors - restarting system");
    rtcErrorCount += consecutiveErrors;
    
    // Save state before restart
    measurementPrefs.putUInt("count", measurementCount);
    measurementPrefs.end();
    
    delay(1000);
    ESP.restart();
  }
  
  // Log status periodically
  logSystemStatus();
}
