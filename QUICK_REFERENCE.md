# Quick Reference: NVS Buffer & WiFi Management

## What Changed?

### Before vs After

| Feature | Before | After |
|---------|--------|-------|
| **Buffer Storage** | RAM (lost on sleep) | NVS (persists forever) |
| **Deep Sleep** | Flush before sleep | No flush (NVS persists) |
| **SD Card Init** | On every boot | Lazy (only when needed) |
| **WiFi Power** | Always on | Auto-disable after 3min |
| **Measurement Count** | Reset on reboot | Persists in NVS |
| **Buffer Flush** | Time-based only | 80% threshold + manual + time |

## Key Code Locations

### NVS Buffer Implementation
**File**: `src/datalogger.h`

```cpp
// Old (RAM buffer)
String buffer[MAX_BUFFER_SIZE];

// New (NVS buffer)
Preferences bufferPrefs;  // Namespace: "databuffer"
// Keys: "count", "d0", "d1", ..., "d99"
```

### WiFi Management
**File**: `src/main.cpp`

```cpp
// New global variables
bool wifiEnabled = true;
unsigned long wifiTimeoutStart = 0;
const unsigned long WIFI_TIMEOUT_MS = 180000;  // 3 minutes

// New functions
void checkWiFiTimeout()    // Check for inactivity
void disableWiFi()         // Power down WiFi
void enableWiFi()          // Restart WiFi
void IRAM_ATTR wifiButtonISR()  // GPIO 0 interrupt
```

### Measurement Persistence
**File**: `src/main.cpp`

```cpp
// New global
Preferences measurementPrefs;  // Namespace: "measurements"

// In setup()
measurementCount = measurementPrefs.getUInt("count", 0);

// In takeMeasurement()
measurementPrefs.putUInt("count", measurementCount);
```

## Usage Examples

### Manual Buffer Flush
1. Open web interface (http://[device-ip]/)
2. Go to Dashboard tab
3. Click "Flush Buffer to SD Card" button
4. Confirm in dialog
5. Data written to SD immediately

### WiFi Re-enable
1. WiFi auto-disables after 3 minutes of no activity
2. Press GPIO 0 button on device
3. WiFi and web server restart
4. Reconnect to web interface

### Check Buffer Status
Dashboard shows:
- "Buffer Status: X / 100" (current buffered entries)
- Auto-flush at 80 entries
- Manual flush available anytime

## Power Savings Calculation

### Typical Scenario
- Measurement interval: 60 seconds
- Deep sleep enabled
- Battery powered (< 5V)

### Power Draw
| Component | Before | After | Savings |
|-----------|--------|-------|---------|
| WiFi (always on) | 100mA | 0mA (after 3min) | 100mA |
| SD card (always init) | 10mA | 0mA (lazy init) | 10mA |
| Active measurement | 50mA | 50mA | 0mA |
| Deep sleep | 0.01mA | 0.01mA | 0mA |

### Battery Life (2000mAh)
- **Before**: ~20-25 hours
- **After**: ~50-80 hours
- **Improvement**: 2-3x longer

## NVS Storage Usage

### Namespaces Used
1. `"omnilogger"` - Device configuration (existing)
2. `"databuffer"` - Buffered data points (new)
3. `"measurements"` - Measurement count (new)

### Buffer Capacity
- Max entries: 100
- Avg entry size: 100 bytes
- Total buffer: ~10KB
- NVS partition: ~20KB
- **Usage**: ~50% of NVS

## Testing Quick Checks

### ✅ NVS Buffer Works
```
1. Enable buffering in settings
2. Take 5 measurements
3. Check dashboard: "Buffer Status: 5 / 100"
4. Power cycle device
5. Check dashboard: "Buffer Status: 5 / 100" (persisted!)
```

### ✅ WiFi Timeout Works
```
1. Boot device
2. Connect to web interface
3. Wait 3 minutes without interaction
4. Check serial: "WiFi disabled"
5. Press GPIO 0 button
6. Check serial: "WiFi re-enabled"
```

### ✅ Measurement Count Persists
```
1. Note count on dashboard
2. Reboot device
3. Count continues from previous value
```

### ✅ Auto-Flush at 80%
```
1. Enable buffering
2. Take 80 measurements
3. Check serial: "Buffer 80% full, triggering flush..."
4. Check SD card for new data
```

## Troubleshooting Quick Fixes

### Buffer not persisting?
```bash
# Check NVS initialization in serial output
# Should see: "Restored X buffered entries from NVS"

# If fails, erase NVS and restart:
pio run --target erase
pio run --target upload
```

### WiFi not disabling?
```bash
# Check serial output for timeout messages
# Verify no clients connected (AP mode)
# Verify network disconnected (STA mode)
```

### SD card fails on flush?
```bash
# Insert SD card before flush
# Format as FAT32
# Check CS pin: GPIO 12 (default)
```

## Web Interface Changes

### New Dashboard Elements
- **WiFi Status**: Shows "✓ Enabled" or "✗ Disabled"
- **Flush Button**: "Flush Buffer to SD Card"
- **Buffer Display**: "X / 100" entries buffered

### New API Endpoint
```
POST /api/flush
Response: {
  "message": "Successfully flushed X data points to SD card",
  "success": true
}
```

## Configuration Notes

### Existing Settings (Unchanged)
- Buffering enable/disable
- Flush interval (seconds)
- Measurement interval
- Deep sleep enable/disable

### New Behavior (No Config Needed)
- 80% auto-flush threshold (hardcoded)
- 3-minute WiFi timeout (hardcoded)
- GPIO 0 for WiFi re-enable (hardcoded)

### Future Config Ideas
- WiFi timeout duration setting
- Buffer threshold percentage setting
- Alternative button for WiFi control

## Important Constants

```cpp
// Buffer
#define MAX_BUFFER_SIZE 100        // Max entries
const int FLUSH_THRESHOLD = 80;    // 80% = 80 entries

// WiFi
const unsigned long WIFI_TIMEOUT_MS = 180000;  // 3 minutes

// NVS Namespaces
"databuffer"    // Buffer data
"measurements"  // Measurement count
"omnilogger"    // Device config (existing)

// GPIO
GPIO 0 = WiFi re-enable button (with pullup)
```

## Compatibility

### ✅ Compatible With
- All existing SD card data files
- All existing configurations
- All existing sensors
- Web interface from any browser

### ⚠️ Notes
- First boot after update: Buffer starts empty
- Measurement count starts from 0 if never set
- GPIO 0 is also boot button (normal behavior)

## Further Reading

- See `IMPLEMENTATION_NOTES.md` for detailed technical info
- See `README.md` for general OmniLogger usage
- See `TROUBLESHOOTING.md` for common issues
- See code comments in source files for specifics
