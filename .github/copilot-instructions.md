# OmniLogger - Copilot Coding Agent Instructions

## Project Overview

**OmniLogger** is a semi-universal data logger system for the Lolin (WEMOS) ESP32-S2 Mini microcontroller. It supports multiple sensor types (BME280, DHT22, DS18B20, analog sensors), logs data to SD card in CSV format, provides a web interface for configuration/monitoring, and includes features like NTP time sync, battery monitoring, and deep sleep power management.

- **Project Type**: Embedded systems firmware (IoT data logger)
- **Platform**: ESP32-S2 Mini (WEMOS/Lolin)
- **Framework**: Arduino (ESP32)
- **Build System**: PlatformIO
- **Language**: C++ (Arduino-flavored)
- **Repository Size**: ~456KB (small, ~2,100 lines of code)
- **Target Hardware**: ESP32-S2 based microcontrollers

## Critical Build Information

### Prerequisites
- **PlatformIO Core** (version 6.1.18 or later)
- Install with: `pip3 install platformio` or `pip install platformio`
- Verify installation: `pio --version`

### Build Commands

**IMPORTANT**: PlatformIO requires internet access during first build to download the ESP32 platform and libraries. Subsequent builds can work offline if dependencies are cached.

#### Build the firmware (compile only):
```bash
pio run
```
- **First build**: Takes 3-5 minutes (downloads ESP32 platform ~500MB, installs libraries)
- **Subsequent builds**: Takes 30-60 seconds
- **Success indicator**: Look for "SUCCESS" at the end, no error messages
- **Common failure**: Network issues during platform/library download - retry if this occurs

#### Install dependencies explicitly (if needed):
```bash
pio pkg install -e lolin_s2_mini
```
- Use this if build fails with missing dependencies

#### Upload to device (requires hardware):
```bash
pio run --target upload
```
- Requires ESP32-S2 board connected via USB
- Will fail in CI/sandboxed environments without hardware

#### Monitor serial output (requires hardware):
```bash
pio device monitor
# Or with specific baud rate:
pio device monitor -b 115200
```

#### Clean build artifacts:
```bash
pio run --target clean
```

#### Erase device flash (requires hardware):
```bash
pio run --target erase
```

### Build Environment Notes

- **Build artifacts**: Created in `.pio/` directory (ignored by git)
- **Platform cache**: `~/.platformio/` (global PlatformIO cache)
- **Build flags**: Defined in `platformio.ini` (includes debug level, PSRAM support)
- **NO TESTS**: This project has no automated test suite
- **NO LINTERS**: No linting configuration (no clang-format, cpplint, etc.)
- **NO CI/CD**: No GitHub Actions or other CI pipelines configured

### Validation Strategy

Since there are no automated tests or linters:
1. **Compile check**: Run `pio run` to verify code compiles without errors
2. **Manual review**: Check code logic and syntax manually
3. **Hardware testing**: If hardware available, upload and test via serial monitor
4. **Documentation**: Verify changes align with README and other docs

## Project Architecture

### Directory Structure
```
OmniLogger/
├── .github/                    # GitHub configuration (copilot instructions)
├── .pio/                       # Build artifacts (gitignored)
├── src/                        # Source code (5 files, ~2,100 lines)
│   ├── main.cpp               # Entry point, setup/loop, main logic (272 lines)
│   ├── config.h               # Configuration management, NVS storage (202 lines)
│   ├── sensors.h              # Sensor drivers and readings (390 lines)
│   ├── datalogger.h           # SD card operations, CSV writing (392 lines)
│   └── webserver.h            # HTTP server, web UI HTML (853 lines)
├── examples/                   # Wiring diagrams, example configs
│   ├── WIRING.md              # Hardware connection details
│   └── CONFIGURATIONS.md      # Example sensor configurations
├── platformio.ini             # PlatformIO build configuration
├── README.md                  # Main documentation (358 lines)
├── QUICKSTART.md              # Quick setup guide (255 lines)
├── CONTRIBUTING.md            # Contribution guidelines (416 lines)
├── TROUBLESHOOTING.md         # Common issues and solutions (818 lines)
├── CHANGELOG.md               # Version history (170 lines)
├── LICENSE                    # MIT License
└── .gitignore                 # Git ignore rules
```

### Key Source Files

#### `src/main.cpp` (272 lines)
- Application entry point
- `setup()`: Initializes config, filesystem, SD card, sensors, WiFi, web server
- `loop()`: Handles measurements, deep sleep, buffering
- Global objects: `deviceConfig`, `sensorManager`, `dataLogger`, `webServer`
- Functions: `setupWiFi()`, `syncTime()`, `takeMeasurement()`, `enterDeepSleep()`, `readBatteryVoltage()`

#### `src/config.h` (202 lines)
- `Config` class: Manages all device settings
- Uses `Preferences` library (ESP32 NVS) for persistent storage
- Settings: WiFi credentials, sensor configs, intervals, pins, timezone
- `SensorConfig` struct: Stores per-sensor configuration (type, pin, name, enabled)
- `SensorType` enum: NONE, BME280, DHT22, DS18B20, ANALOG
- Namespace: "omnilogger" (NVS partition name)

#### `src/sensors.h` (390 lines)
- `SensorManager` class: Handles all sensor operations
- Manages up to 8 sensors simultaneously (Config::MAX_SENSORS = 8)
- Sensor drivers: BME280 (I2C), DHT22, DS18B20 (OneWire), Analog (ADC)
- Dynamic memory management for sensor objects (DHT*, OneWire*, DallasTemperature*)
- Methods: `begin()`, `readAllSensors()`, `getCSVHeader()`, `getCSVData()`, `cleanup()`
- Important: Always calls `cleanup()` to prevent memory leaks

#### `src/datalogger.h` (392 lines)
- `DataLogger` class: SD card operations
- Features: CSV writing, file rotation, buffering, storage stats
- File naming: `data_YYYYMMDD.csv` (one file per day)
- Buffer: In-memory buffer (up to 100 entries) for reducing SD wear
- Methods: `begin()`, `writeData()`, `flush()`, `listFiles()`, `downloadFile()`, `getStorageInfo()`
- Uses SPI interface (CS pin configurable, default GPIO 12)

#### `src/webserver.h` (853 lines, largest file)
- `WebServerManager` class: HTTP server and web UI
- All HTML/CSS/JavaScript embedded as C++ strings (no external files)
- Tabs: Dashboard, Sensors, Settings, Data
- REST-like endpoints: `/api/status`, `/api/sensors/config`, `/api/settings`, `/api/data/list`, `/api/data/download`
- Dashboard auto-refreshes every 5 seconds
- Note: Contains one TODO at line 799 for data retrieval implementation

### Dependencies (from platformio.ini)

All automatically installed by PlatformIO:
- `adafruit/Adafruit BME280 Library@^2.2.2` - BME280 sensor driver
- `adafruit/Adafruit Unified Sensor@^1.1.14` - Adafruit sensor abstraction
- `adafruit/DHT sensor library@^1.4.4` - DHT22 sensor driver
- `paulstoffregen/OneWire@^2.3.7` - OneWire protocol (DS18B20)
- `milesburton/DallasTemperature@^3.11.0` - DS18B20 driver
- `bblanchon/ArduinoJson@^6.21.3` - JSON parsing/generation

### Pin Configuration (ESP32-S2 Mini defaults)

Critical hardware pin assignments (defined in `src/config.h`):
- **SD Card CS**: GPIO 12 (configurable, DEFAULT_SD_CS)
- **SD Card SCK**: GPIO 7 (SPI, hardware fixed)
- **SD Card MISO**: GPIO 9 (SPI, hardware fixed)
- **SD Card MOSI**: GPIO 11 (SPI, hardware fixed)
- **I2C SDA**: GPIO 33 (configurable, DEFAULT_I2C_SDA)
- **I2C SCL**: GPIO 35 (configurable, DEFAULT_I2C_SCL)
- **Battery ADC**: GPIO 1 (default, configurable)
- **Digital sensors**: Any available GPIO (user-configured via web interface)

### Configuration Storage

- Uses ESP32 **Preferences** library (NVS - Non-Volatile Storage)
- Namespace: "omnilogger"
- Persists across reboots and firmware updates (unless explicitly erased)
- To reset: `pio run --target erase` then re-upload
- Storage keys: Various (wifi_ssid, wifi_pass, ap_ssid, measurement_interval, etc.)

## Common Coding Patterns

### Memory Management
- **Critical**: Always clean up dynamically allocated sensor objects
- Use `cleanup()` method before reinitializing sensors
- Pattern in `sensors.h`: Delete old objects, set to nullptr, create new

### String Handling
- Use `char` arrays for fixed-size strings (e.g., `char wifiSSID[64]`)
- Use `String` class for dynamic/temporary strings
- `strcpy()` for copying strings to char arrays
- `snprintf()` for safe formatted strings

### Error Handling
- Check return values of initialization functions
- Print error messages to `Serial` (115200 baud)
- Graceful degradation (e.g., continue without SD card if init fails)
- No exceptions (Arduino doesn't use C++ exceptions)

### WiFi Patterns
- Try Station (STA) mode first with configured credentials
- Fall back to Access Point (AP) mode if connection fails
- Default AP: SSID "OmniLogger", password "omnilogger123"
- AP IP always: 192.168.4.1

## Making Changes

### Adding a New Sensor Type

1. **Update `src/config.h`**:
   - Add new value to `SensorType` enum (e.g., `SENSOR_SHT31 = 5`)

2. **Update `src/sensors.h`**:
   - Add initialization method (e.g., `initSHT31()`)
   - Add reading method (e.g., `readSHT31()`)
   - Add case to `begin()` switch statement
   - Add case to `readAllSensors()` switch statement
   - Update `getCSVHeader()` for new data fields
   - Update `getCSVData()` for new data values
   - Add member variables for sensor object if needed

3. **Update `src/webserver.h`**:
   - Add `<option>` to sensor type dropdown in HTML (search for "select" tag)
   - Ensure value matches enum value

4. **Update dependencies**:
   - If new library needed, add to `lib_deps` in `platformio.ini`
   - Include new header in `sensors.h`

5. **Update documentation**:
   - Add to README.md Features section
   - Add to examples/WIRING.md with connection details
   - Update CONTRIBUTING.md if needed

### Modifying Web Interface

- All HTML is in `src/webserver.h` as C++ string literals
- Use raw string literals for multi-line HTML: `R"rawdelimiter( ... )rawdelimiter"`
- JavaScript is inline in the HTML
- CSS is inline in `<style>` tags
- To test: Build, upload to device, access via browser at device IP

### Changing Configuration Options

1. Add field to `Config` class in `src/config.h`
2. Initialize in `Config()` constructor
3. Add load/save logic in `load()` and `save()` methods using Preferences
4. Add corresponding input fields in `src/webserver.h` Settings tab
5. Handle in `/api/settings` POST handler

### File Size Constraints

- ESP32-S2 has limited flash (typically 4MB)
- LittleFS partition for web files (configured in platformio.ini)
- Large HTML/JS/CSS strings increase binary size
- Monitor compiled size: Check "Firmware size" in build output
- Max firmware size shown at end of build (typically ~1.2MB limit)

## Important Conventions

### Code Style
- **Indentation**: 2 spaces (not tabs)
- **Braces**: Opening brace on same line (`if (condition) {`)
- **Naming**:
  - Classes: PascalCase (`SensorManager`, `DataLogger`)
  - Functions/methods: camelCase (`readAllSensors()`, `getCSVData()`)
  - Variables: camelCase (`measurementInterval`, `sensorTypes`)
  - Constants/macros: UPPER_SNAKE_CASE (`DEFAULT_SD_CS`, `MAX_SENSORS`)
  - Member variables: camelCase, no prefix
- **Comments**: Use `//` for single line, `/* */` for multi-line
- **Serial output**: Use `Serial.println()` and `Serial.printf()` for debugging

### Arduino Specifics
- Use `setup()` and `loop()` functions (required by Arduino framework)
- Use Arduino types: `String`, `byte`, etc.
- Use Arduino functions: `pinMode()`, `digitalWrite()`, `analogRead()`, `delay()`, `millis()`
- Timing: Use `millis()` for non-blocking delays, avoid `delay()` in production code
- Serial: Always `Serial.begin(115200)` in setup, 115200 is standard baud rate

### ESP32 Specifics
- Use `Preferences` for persistent storage (not EEPROM on ESP32)
- WiFi: Include `<WiFi.h>` and use `WiFi.begin()`, `WiFi.status()`, etc.
- Deep sleep: Use `esp_deep_sleep_start()` and related functions
- GPIO: Valid pins vary by board, ESP32-S2 has specific limitations
- ADC: Use ADC1 pins (GPIO 1-10) when WiFi is active (ADC2 unavailable with WiFi)

## Known Issues and Workarounds

### Memory Leaks with Sensors
- **Issue**: Previous implementation had memory leaks when reconfiguring sensors
- **Solution**: Always call `cleanup()` before reinitializing sensors in `SensorManager`
- **Pattern**: Delete all sensor objects, set pointers to nullptr, then create new ones
- See recent commit history for fix details

### SD Card Compatibility
- **Issue**: Some SD cards don't work with SPI mode
- **Solution**: Use standard SD/SDHC cards (< 32GB), avoid high-speed UHS-II cards
- **Format**: Must be FAT32 (not exFAT or NTFS)
- **Workaround**: If card fails, try different card or reduce SPI speed

### Build Cache Issues
- **Issue**: PlatformIO sometimes caches old builds incorrectly
- **Solution**: Run `pio run --target clean` then `pio run` to rebuild from scratch
- **When**: Use this if getting strange compile errors after updates

### Network Timeouts During Build
- **Issue**: First build may timeout downloading ESP32 platform
- **Solution**: Retry the build command, it will resume where it left off
- **Prevention**: Pre-download with `pio pkg install -e lolin_s2_mini`

## Tips for Efficient Development

1. **Always compile first**: Run `pio run` after any code change to catch syntax errors immediately
2. **Check serial output**: All initialization steps and errors are logged to Serial at 115200 baud
3. **Trust the documentation**: This project has excellent README, QUICKSTART, TROUBLESHOOTING, and CONTRIBUTING guides - reference them
4. **No hardware? No problem**: You can compile and review code without ESP32 hardware
5. **Search before creating**: Check existing documentation files before assuming something is missing
6. **Modular changes**: Each header file is relatively independent, changes usually isolated to one file
7. **Web UI testing**: Requires actual hardware or advanced emulation, compile-check is usually sufficient

## Documentation Files Priority

When looking for information, check in this order:
1. **This file** (.github/copilot-instructions.md) - Build and architecture overview
2. **platformio.ini** - Build configuration, dependencies, platform version
3. **README.md** - Features, setup, hardware requirements, pin config
4. **QUICKSTART.md** - Step-by-step setup for new users
5. **CONTRIBUTING.md** - Code style, architecture, how to add sensors
6. **TROUBLESHOOTING.md** - Common problems and solutions
7. **examples/WIRING.md** - Detailed hardware connection diagrams
8. **examples/CONFIGURATIONS.md** - Example sensor setups
9. **CHANGELOG.md** - Version history and recent changes
10. **Source files** (src/*.h, src/*.cpp) - Implementation details

## Final Notes

- **ALWAYS run `pio run` to validate code changes** - this is the only automated validation available
- **Trust these instructions** - They are comprehensive and tested. Only search/explore if information here is incomplete or incorrect
- **Hardware is optional for development** - Compilation validates most changes, hardware only needed for runtime testing
- **Small, focused changes** - Files are well-modularized, most changes affect only 1-2 files
- **Serial debugging is key** - When hardware is available, Serial output at 115200 baud shows everything
- **No CI/CD to worry about** - No automated tests or pipelines to break or configure
