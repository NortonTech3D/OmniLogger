# Architecture Diagrams: Before vs After

## Data Flow: Before Implementation

```
┌─────────────┐
│   Setup()   │
└──────┬──────┘
       │
       ├─── Initialize SD Card (always)
       ├─── Initialize WiFi (always on)
       └─── Start Web Server
              │
┌─────────────┴──────────────┐
│        Main Loop           │
│                            │
│  ┌──────────────────────┐ │
│  │  Take Measurement    │ │
│  └──────────┬───────────┘ │
│             │              │
│  ┌──────────▼───────────┐ │
│  │  Write to RAM Buffer │ │ <- Lost on sleep!
│  └──────────┬───────────┘ │
│             │              │
│  ┌──────────▼───────────┐ │
│  │  Flush Every 5 min   │ │
│  │  to SD Card          │ │
│  └──────────────────────┘ │
│                            │
│  ┌──────────────────────┐ │
│  │  Enter Deep Sleep    │ │
│  │  ├─ FLUSH BUFFER!    │ │ <- Defeats buffering
│  │  └─ Sleep Timer      │ │
│  └──────────────────────┘ │
│                            │
│  WiFi: Always ON          │ <- Wastes power
└────────────────────────────┘
```

## Data Flow: After Implementation

```
┌─────────────┐
│   Setup()   │
└──────┬──────┘
       │
       ├─── Load Measurement Count from NVS
       ├─── Load Buffer from NVS (restored!)
       ├─── Initialize WiFi (with timeout)
       ├─── Set GPIO 0 Interrupt
       └─── Start Web Server
              │
┌─────────────┴──────────────┐
│        Main Loop           │
│                            │
│  ┌──────────────────────┐ │
│  │  Take Measurement    │ │
│  └──────────┬───────────┘ │
│             │              │
│  ┌──────────▼───────────┐ │
│  │ Write to NVS Buffer  │ │ <- Persists forever!
│  │ (keys: d0, d1, ...)  │ │
│  └──────────┬───────────┘ │
│             │              │
│  ┌──────────▼───────────┐ │
│  │  Save Measurement    │ │
│  │  Count to NVS        │ │
│  └──────────┬───────────┘ │
│             │              │
│  ┌──────────▼───────────┐ │
│  │  Check Buffer Size   │ │
│  │  If >= 80%:          │ │
│  │  ├─ Init SD (lazy)   │ │ <- Only when needed
│  │  ├─ Flush to SD      │ │
│  │  └─ Clear NVS Buffer │ │
│  └──────────────────────┘ │
│                            │
│  ┌──────────────────────┐ │
│  │  Check WiFi Timeout  │ │
│  │  If > 3 min idle:    │ │
│  │  └─ Disable WiFi     │ │ <- Power savings!
│  └──────────────────────┘ │
│                            │
│  ┌──────────────────────┐ │
│  │  Enter Deep Sleep    │ │
│  │  ├─ NO FLUSH         │ │ <- Buffer in NVS
│  │  └─ Sleep Timer      │ │
│  └──────────────────────┘ │
│                            │
│  WiFi: Auto-disable       │ <- Saves power
│  GPIO 0: Re-enable WiFi   │
└────────────────────────────┘
```

## NVS Storage Layout

```
┌────────────────────────────────────────┐
│         ESP32-S2 NVS Partition         │
│              (~20-24 KB)               │
├────────────────────────────────────────┤
│                                        │
│  Namespace: "omnilogger"               │
│  ┌──────────────────────────────────┐ │
│  │ wifiSSID          : "MyNetwork"  │ │
│  │ wifiPass          : "********"   │ │
│  │ apSSID            : "OmniLogger" │ │
│  │ measurementInt    : 60           │ │
│  │ deepSleep         : true         │ │
│  │ bufferingEnabled  : true         │ │
│  │ ... (other config)               │ │
│  └──────────────────────────────────┘ │
│                                        │
│  Namespace: "databuffer" <- NEW!       │
│  ┌──────────────────────────────────┐ │
│  │ count : 42                       │ │
│  │ d0    : "2026-01-04 10:15:23,..." │ │
│  │ d1    : "2026-01-04 10:16:23,..." │ │
│  │ d2    : "2026-01-04 10:17:23,..." │ │
│  │ ...                              │ │
│  │ d41   : "2026-01-04 11:00:23,..." │ │
│  └──────────────────────────────────┘ │
│                                        │
│  Namespace: "measurements" <- NEW!     │
│  ┌──────────────────────────────────┐ │
│  │ count : 12847                    │ │
│  └──────────────────────────────────┘ │
│                                        │
└────────────────────────────────────────┘
```

## WiFi State Machine

```
┌─────────────┐
│    BOOT     │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────┐
│     WiFi ENABLED                │
│  - Web server running           │
│  - Timeout timer started        │
│  - Monitoring for activity      │
└────────┬─────────────┬──────────┘
         │             │
         │ 3min        │ Activity
         │ idle        │ detected
         │             │
         ▼             ▼
┌────────────────┐    (reset timer)
│ WiFi DISABLED  │
│  - Power saved │
│  - Web offline │
│  - Waiting...  │
└────────┬───────┘
         │
         │ GPIO 0
         │ pressed
         │
         ▼
    (return to
     ENABLED)
```

## Buffer Flush Triggers

```
┌─────────────────────────────────────┐
│        Buffer Flush Triggers        │
└─────────────────────────────────────┘

Trigger 1: 80% FULL (NEW!)
┌─────────────────────────────────┐
│ Buffer: [■■■■■■■■□□]            │
│         80/100 entries          │
│         ↓ AUTO FLUSH            │
└─────────────────────────────────┘

Trigger 2: Manual Flush (NEW!)
┌─────────────────────────────────┐
│ User clicks button on dashboard │
│         ↓ IMMEDIATE FLUSH       │
└─────────────────────────────────┘

Trigger 3: Time Interval
┌─────────────────────────────────┐
│ flushInterval seconds elapsed   │
│ (default: 300 = 5 minutes)      │
│         ↓ SCHEDULED FLUSH       │
└─────────────────────────────────┘

All triggers:
  ├─ Initialize SD if needed
  ├─ Read all entries from NVS
  ├─ Write to SD card CSV
  └─ Clear NVS buffer
```

## Power Consumption Timeline

### Before Implementation
```
Time:    0s      60s     120s    180s    240s
         │       │       │       │       │
WiFi:    ████████████████████████████████  (100mA)
SD:      ████████████████████████████████  (10mA)
MCU:     ████████████████████████████████  (50mA)
Sleep:   No deep sleep possible
         ────────────────────────────────
Total:   ~160mA continuous
Battery: ~12 hours (2000mAh)
```

### After Implementation
```
Time:    0s      60s     120s    180s    240s
         │       │       │       │       │
WiFi:    ████████████████░░░░░░░░░░░░░░  (100mA → 0mA @ 3min)
SD:      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  (0mA, lazy init)
MCU:     █░░░░░░░█░░░░░░░█░░░░░░░█░░░░░░  (50mA → 0.01mA in sleep)
Sleep:   ░███████░███████░███████░███████  (deep sleep between)
         ────────────────────────────────
Avg:     ~20mA (with deep sleep)
Battery: ~100 hours (2000mAh)
Gain:    8x improvement!
```

## Web Interface Changes

### Dashboard: Before
```
┌─────────────────────────────────────┐
│          Dashboard                  │
├─────────────────────────────────────┤
│  Data Points: 1,234                 │
│  Battery: 3.87V                     │
│  Storage: 45MB / 256MB              │
│  SD Health: ✓ Healthy               │
│  Active Sensors: 3                  │
│  Uptime: 2h 15m                     │
│  Buffer: 5 / 100                    │
│                                     │
│  Current Readings:                  │
│  [sensor data...]                   │
└─────────────────────────────────────┘
```

### Dashboard: After
```
┌─────────────────────────────────────┐
│          Dashboard                  │
├─────────────────────────────────────┤
│  Data Points: 1,234                 │
│  Battery: 3.87V                     │
│  Storage: 45MB / 256MB              │
│  SD Health: ✓ Healthy               │
│  Active Sensors: 3                  │
│  Uptime: 2h 15m                     │
│  Buffer: 5 / 100                    │
│  WiFi Status: ✓ Enabled    <- NEW!  │
│                                     │
│  [Flush Buffer to SD Card]  <- NEW! │
│                                     │
│  Current Readings:                  │
│  [sensor data...]                   │
└─────────────────────────────────────┘
```

## SD Card File Structure

### CSV Files (Unchanged)
```
SD Card Root:
├── data_20260104.csv    <- Today's data
├── data_20260103.csv    <- Yesterday
├── data_20260102.csv
└── ...

data_20260104.csv:
┌────────────────────────────────────────────┐
│ Timestamp,Temp,Humidity,Pressure,...       │
│ 2026-01-04 10:15:23,22.5,45.2,1013.2,...  │
│ 2026-01-04 10:16:23,22.6,45.1,1013.3,...  │
│ 2026-01-04 10:17:23,22.5,45.3,1013.2,...  │
│ ...                                        │
└────────────────────────────────────────────┘
```

## Memory Usage Comparison

### Before (RAM Buffer)
```
RAM Usage:
  Buffer: 100 × 150 bytes = 15,000 bytes
  LOST on deep sleep!

NVS Usage:
  Config only: ~1 KB
```

### After (NVS Buffer)
```
RAM Usage:
  Minimal: ~100 bytes
  PRESERVED in deep sleep!

NVS Usage:
  Config: ~1 KB
  Buffer: ~12 KB (100 entries × ~120 bytes)
  Measurement count: 16 bytes
  Total: ~13 KB (well within 20KB partition)
```

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────┐
│               ESP32-S2 System                   │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌──────────┐      ┌─────────────┐            │
│  │  Sensors ├─────→│ SensorMgr   │            │
│  └──────────┘      └──────┬──────┘            │
│                            │                    │
│  ┌──────────┐      ┌──────▼──────┐            │
│  │   NVS    │◄────→│ DataLogger  │            │
│  │ "buffer" │      │  - bufferTo │            │
│  │          │      │    NVS()    │            │
│  └──────────┘      │  - flushTo  │            │
│                    │    SD()     │            │
│  ┌──────────┐      └──────┬──────┘            │
│  │ SD Card  │◄────────────┘                   │
│  └──────────┘                                  │
│                                                 │
│  ┌──────────┐      ┌─────────────┐            │
│  │   WiFi   │◄────→│  WiFi Mgr   │            │
│  │  Radio   │      │  - timeout  │            │
│  └──────────┘      │  - enable   │            │
│                    │  - disable  │            │
│  ┌──────────┐      └──────┬──────┘            │
│  │ GPIO 0   ├─────────────┘ ISR               │
│  │  Button  │                                  │
│  └──────────┘                                  │
│                                                 │
│  ┌──────────┐      ┌─────────────┐            │
│  │   HTTP   │◄────→│ WebServer   │            │
│  │  Client  │      │  - status   │            │
│  └──────────┘      │  - flush    │            │
│                    └─────────────┘            │
│                                                 │
│  ┌──────────────────────────────┐             │
│  │      Deep Sleep Timer        │             │
│  │   - NO flush before sleep    │             │
│  │   - Buffer persists in NVS   │             │
│  └──────────────────────────────┘             │
│                                                 │
└─────────────────────────────────────────────────┘
```
