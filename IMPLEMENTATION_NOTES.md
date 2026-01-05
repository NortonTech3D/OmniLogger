# NVS Buffer and WiFi Management Implementation Notes

## Overview
This document describes the implementation of non-volatile storage (NVS) for data buffering and intelligent WiFi power management in the OmniLogger firmware.

## Implementation Date
2026-01-04

## Changes Summary

### 1. NVS-Based Persistent Buffer (`src/datalogger.h`)

**Problem**: RAM-based buffer was lost on deep sleep and power loss.

**Solution**: Replaced with NVS (Preferences library) storage.

**Key Changes**:
- Added `Preferences bufferPrefs` member using namespace "databuffer"
- Removed `String buffer[MAX_BUFFER_SIZE]` array
- Modified constructor to load `bufferCount` from NVS on boot
- Modified `logData()` to store each data point in NVS with keys "d0", "d1", etc.
- Added 80% buffer threshold - auto-flush when buffer reaches 80 entries (out of 100)
- Modified `flushBuffer()` to read from NVS, write to SD, then clear NVS entries
- Buffer data now persists across:
  - Deep sleep cycles
  - Power loss
  - Device resets

**NVS Keys Used**:
- `count`: Current number of buffered entries (uint32_t)
- `d0`, `d1`, `d2`, ..., `d99`: Individual data point strings

### 2. Lazy SD Card Initialization (`src/datalogger.h`)

**Problem**: SD card initialized on every boot/wake, wasting power.

**Solution**: Delay initialization until first flush operation.

**Key Changes**:
- Added `sdInitialized` boolean flag
- Modified `begin()` to NOT initialize SD card, only set up pins
- Created new `initSDCard()` method with all SD init logic
- Modified `writeToSD()` to call `initSDCard()` if not yet initialized
- Modified `flushBuffer()` to call `initSDCard()` before flushing
- Other SD operations check `sdInitialized` instead of `initialized`

**Benefits**:
- Saves power when only buffering to NVS
- Reduces wear on SD card
- Faster boot time

### 3. Measurement Count Persistence (`src/main.cpp`)

**Problem**: Measurement count reset to 0 on every reboot.

**Solution**: Store in NVS using Preferences.

**Key Changes**:
- Added global `Preferences measurementPrefs`
- In `setup()`: Load count from NVS namespace "measurements", key "count"
- In `takeMeasurement()`: Save count after each successful measurement
- Count now persists across all resets

### 4. Deep Sleep Behavior Modification (`src/main.cpp`)

**Problem**: Forced flush before deep sleep defeated purpose of buffering.

**Solution**: Remove flush, rely on NVS persistence.

**Key Changes**:
- Modified `enterDeepSleep()` to remove `dataLogger.flush()` call
- Added comment explaining buffer persists in NVS
- Buffer will only flush when:
  - 80% full (80 entries)
  - Manual flush via web interface
  - Periodic flush interval reached (if configured)

### 5. WiFi Power Management (`src/main.cpp`)

**Problem**: WiFi always on wasted power in battery mode.

**Solution**: Implement 3-minute timeout with manual re-enable.

**Key Changes Added**:

#### Global Variables:
```cpp
bool wifiEnabled = true;
unsigned long wifiTimeoutStart = 0;
const unsigned long WIFI_TIMEOUT_MS = 180000;  // 3 minutes
volatile bool wifiReenableRequested = false;
```

#### Functions:
- `checkWiFiTimeout()`: Monitors WiFi activity
  - Checks for AP clients (`WiFi.softAPgetStationNum()`)
  - Checks for STA connection (`WiFi.status()`)
  - Resets timer on activity
  - Calls `disableWiFi()` after 3min of inactivity

- `disableWiFi()`: Powers down WiFi
  - Disconnects WiFi
  - Sets mode to `WIFI_OFF`
  - Sets `wifiEnabled = false`

- `enableWiFi()`: Restarts WiFi
  - Calls `setupWiFi()`
  - Restarts web server
  - Resets timeout timer
  - Syncs time if connected

- `wifiButtonISR()`: GPIO 0 interrupt handler
  - Sets `wifiReenableRequested` flag
  - Actual work done in main loop (can't do WiFi ops in ISR)

#### Setup Changes:
- Configure GPIO 0 as input with pullup
- Attach falling-edge interrupt to `wifiButtonISR()`
- Start timeout timer

#### Loop Changes:
- Check `wifiReenableRequested` flag and call `enableWiFi()`
- Only call `webServer.handleClient()` if `wifiEnabled`
- Only call `checkWiFiTimeout()` if `wifiEnabled`
- Only resync time if `wifiEnabled`

### 6. Web Interface Updates (`src/webserver.h`)

**New Features**:

#### Manual Flush Endpoint:
- Route: `POST /api/flush`
- Handler: `handleFlushBuffer()`
- Returns JSON: `{message: "...", success: true/false}`
- Flushes all buffered data to SD card immediately

#### Dashboard Updates:
- Added "WiFi Status" stat card showing enabled/disabled
- Added "Flush Buffer to SD Card" button
- JavaScript `flushBuffer()` function:
  - Confirms with user
  - POSTs to `/api/flush`
  - Shows result in alert
  - Refreshes dashboard

#### Status API Update:
- Added `wifiEnabled` field to `/api/status` response
- WebServerManager constructor accepts `getWiFiEnabled` function pointer
- `begin()` signature updated to accept WiFi status callback

## Testing Recommendations

### 1. NVS Buffer Persistence
```
Test Steps:
1. Enable buffering in settings
2. Take several measurements (< 80 to avoid auto-flush)
3. Check dashboard shows buffered count
4. Power cycle device or trigger deep sleep
5. On wake, verify buffer count restored
6. Manually flush via web UI
7. Verify data written to SD card
```

### 2. WiFi Timeout
```
Test Steps:
1. Boot device and connect to web interface
2. Leave interface idle for 3+ minutes
3. Verify WiFi disables (check serial output)
4. Press GPIO 0 button
5. Verify WiFi re-enables
6. Verify web interface accessible again
```

### 3. Lazy SD Init
```
Test Steps:
1. Enable buffering
2. Remove SD card
3. Power on device
4. Verify boot succeeds (check serial - no SD init)
5. Take measurements (should buffer to NVS)
6. Insert SD card
7. Trigger manual flush
8. Verify SD initializes and data written
```

### 4. Measurement Count
```
Test Steps:
1. Note current measurement count
2. Take a few measurements
3. Power cycle device
4. Verify count continues from previous value
```

### 5. Deep Sleep
```
Test Steps:
1. Enable deep sleep and buffering
2. Configure short measurement interval
3. Power via battery (<5V on battery pin)
4. Take measurement, enter deep sleep
5. On wake, verify buffer not flushed
6. After several sleep cycles, check buffer fills
7. Verify auto-flush at 80% capacity
```

## Power Consumption Impact

**WiFi Disabled**: ~80-100mA savings (ESP32-S2)
**SD Card Not Initialized**: ~5-10mA savings
**Reduced SD Writes**: Extends battery life, reduces card wear

**Estimated Battery Life Improvement** (with 2000mAh battery):
- Before: ~20-25 hours (WiFi always on, frequent SD writes)
- After: ~50-80 hours (WiFi timeout, NVS buffering, lazy SD init)

## NVS Storage Considerations

**NVS Partition Size**: Typically 20-24KB on ESP32-S2
**Per Entry Overhead**: ~16 bytes per key
**Data Entry Size**: Variable (depends on sensor count, typically 50-150 bytes)

**Buffer Capacity Calculation**:
- 100 entries × 100 bytes average = 10KB data
- 100 entries × 16 bytes overhead = 1.6KB
- Total: ~11.6KB (well within NVS limits)

**NVS Wear Leveling**: ESP32 NVS has built-in wear leveling, safe for frequent writes

## Known Limitations

1. **Buffer Size**: Fixed at 100 entries (NVS size constraint)
2. **WiFi Timeout**: Fixed at 3 minutes (could be made configurable)
3. **GPIO 0 Button**: Shared with boot button, may interfere with firmware updates
4. **No Web Server Graceful Shutdown**: Calling `begin()` multiple times may have side effects

## Future Enhancements

1. **Configurable WiFi Timeout**: Add to settings page
2. **Configurable Buffer Threshold**: Allow user to set flush percentage
3. **Buffer Compression**: Compress data before storing in NVS to increase capacity
4. **WiFi Status LED**: Visual indicator of WiFi state
5. **Low Battery Auto-Disable WiFi**: Disable WiFi when battery < threshold
6. **Selective Flush**: Flush only specific sensors' data

## Compatibility Notes

- **ESP32 Arduino Core**: Tested with Preferences library (included in core)
- **Existing Configurations**: No migration needed, NVS buffers start empty
- **SD Card**: Backward compatible with existing CSV files
- **Web Interface**: All existing features remain functional

## Code Review Checklist

- [x] NVS namespaces don't conflict ("databuffer", "measurements", "omnilogger")
- [x] All NVS keys within 15-character limit
- [x] Error handling for NVS operations
- [x] Memory leaks prevented (no dynamic allocation in buffer code)
- [x] Interrupt handler is IRAM_ATTR and doesn't do heavy work
- [x] WiFi timeout handles millis() rollover
- [x] Thread safety (ISR only sets flag, work done in loop)
- [x] Backward compatibility maintained

## Build Instructions

```bash
# Install PlatformIO
pip3 install platformio

# Build firmware
cd /path/to/OmniLogger
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

## Troubleshooting

### Buffer Not Persisting
- Check serial output for NVS initialization errors
- Verify NVS partition exists (platformio.ini board configuration)
- Try erasing flash: `pio run --target erase`

### WiFi Not Disabling
- Check serial output for timeout messages
- Verify GPIO 0 not shorted to ground
- Check client connection status in serial logs

### SD Card Init Fails on Flush
- Verify SD card inserted
- Check SD card format (FAT32)
- Verify CS pin configuration matches hardware

### Measurement Count Not Saving
- Check NVS namespace initialization in serial output
- Verify successful measurement logging (count only saved on success)

## References

- ESP32 Preferences Library: https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences
- ESP32 NVS Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
- ESP32 Deep Sleep: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
