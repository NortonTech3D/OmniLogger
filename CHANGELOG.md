# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-04

### Added

#### Core Features
- Initial public release of OmniLogger data logger system
- Support for Lolin (WEMOS) ESP32-S2 Mini microcontroller
- Multi-sensor support architecture (up to 8 sensors simultaneously)
- SD card data logging with CSV format
- Web-based configuration and monitoring interface
- NTP time synchronization with configurable timezone
- Battery voltage monitoring with voltage divider support
- Deep sleep power management for battery operation
- Data buffering feature to reduce SD card wear

#### Sensor Support
- BME280 environmental sensor (I2C)
  - Temperature measurement (-40°C to +85°C)
  - Humidity measurement (0-100%)
  - Barometric pressure measurement (300-1100 hPa)
  - Range validation for reliable readings
- DHT22 temperature and humidity sensor
  - Temperature range: -40°C to +80°C
  - Humidity range: 0-100%
- DS18B20 OneWire temperature sensor
  - Temperature range: -55°C to +125°C
  - Device disconnection detection
- Generic analog sensor support (ADC-based)
  - 12-bit ADC resolution (0-4095)
  - 0-3.3V input range

#### Web Interface
- Responsive dashboard with real-time statistics
  - Data point counter
  - Battery voltage display with validation
  - Storage usage tracking
  - SD card health monitoring
  - Active sensor count
  - System uptime (handles millis() rollover)
  - Current sensor readings with auto-refresh
  - Buffer status display
- Sensor configuration page
  - Enable/disable individual sensors
  - Configure sensor types (BME280, DHT22, DS18B20, Analog)
  - Set GPIO pins for digital sensors with validation
  - Custom sensor naming (up to 31 characters)
- Settings page
  - WiFi configuration (SSID and password)
  - Access Point configuration with password validation
  - Data buffering enable/disable
  - Buffer flush interval configuration
  - Measurement interval adjustment (1s to 24h)
  - Deep sleep mode toggle
  - Timezone offset configuration (-12 to +14)
  - Device reboot function
- Data management page
  - File listing with sizes
  - CSV file download functionality
  - Data retrieval API with JSON output

#### Data Logging
- Automatic CSV file creation with date-based naming (data_YYYYMMDD.csv)
- Configurable measurement intervals (1 second to 24 hours)
- Accurate timestamps for all data points
- Dynamic CSV headers based on sensor configuration
- Daily automatic file rotation
- Storage space monitoring
- In-memory buffering (up to 100 data points)
- Configurable flush intervals for buffer

#### WiFi Features
- Access Point (AP) mode for initial configuration
  - Default SSID: "OmniLogger"
  - Default password: "omnilogger123" (WPA2-compliant, 8+ chars)
- Station (STA) mode for network connectivity
- Automatic fallback to AP mode if connection fails
- Configurable WiFi credentials via web interface
- 2.4GHz WiFi support (ESP32-S2 limitation)

#### Power Management
- Battery voltage monitoring via ADC with voltage divider
- Deep sleep mode for battery operation
- Configurable sleep intervals
- Automatic wake and measurement cycles
- USB power detection (5V indication)
- Pre-sleep buffer flush

#### Configuration System
- Persistent configuration storage using NVS (Preferences)
- Web-based configuration interface
- Default values for first-time setup
- Input validation for all configurable parameters
- Configuration save/load with error handling

#### Security Features
- Input validation for all user inputs
- Path traversal protection in file operations
- JSON validation before parsing
- Bounds checking for array accesses
- Safe string handling with null termination
- Password length validation (WPA2 requirements)
- File size limits for API endpoints

#### Documentation
- Comprehensive README with setup instructions and security section
- Quick Start Guide for rapid deployment (5 steps)
- Detailed wiring diagrams and pin configurations (WIRING.md)
- Example configurations for common use cases (CONFIGURATIONS.md)
- Troubleshooting guide with solutions (TROUBLESHOOTING.md)
- Contributing guidelines for developers (CONTRIBUTING.md)
- AGPL-3.0 License with copyright headers in all source files

### Technical Details

#### Hardware Support
- ESP32-S2 based boards (Lolin S2 Mini)
- I2C sensors on configurable pins (default SDA: GPIO33, SCL: GPIO35)
- SPI SD card interface (default CS: GPIO12, SCK: GPIO7, MISO: GPIO9, MOSI: GPIO11)
- ADC inputs for analog sensors (GPIO1-10, 12-bit resolution)
- Digital sensor inputs on any available GPIO (0-45)

#### Software Architecture
- Modular design with separate components:
  - `config.h`: Configuration management
  - `sensors.h`: Sensor interface and implementations
  - `datalogger.h`: SD card operations
  - `webserver.h`: HTTP server and web UI
- REST API for programmatic access
- JSON-based data exchange
- Embedded web interface (no external files needed)

#### Dependencies
- Arduino framework for ESP32
- Adafruit BME280 Library (v2.2.2+)
- Adafruit Unified Sensor (v1.1.14+)
- DHT sensor library (v1.4.4+)
- OneWire (v2.3.7+)
- DallasTemperature (v3.11.0+)
- ArduinoJson (v6.21.3+)

#### Build System
- PlatformIO configuration
- ESP32 platform support
- LittleFS filesystem support
- Automated dependency management

### Known Limitations

- Maximum 8 sensors simultaneously
- SD card must be FAT32 format
- WiFi 2.4GHz only (no 5GHz support)
- No HTTPS support (HTTP only)
- No authentication on web interface
- Single sensor per type (except multiple I2C devices)

### Future Enhancements

Planned for future releases:
- MQTT support for cloud integration
- InfluxDB compatibility
- Graphical data visualization in web interface
- Email/SMS alerts for threshold violations
- OTA (Over-The-Air) firmware updates
- User authentication for web interface
- Data export in additional formats (JSON, XML)
- Real-time graphing in dashboard
- Mobile app companion
- Support for additional sensor types

---

## Version History

### Version Numbering

This project uses [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: New functionality (backwards compatible)
- **PATCH**: Bug fixes (backwards compatible)

### Release Types

- **Stable**: Production-ready releases
- **Beta**: Feature-complete but needs testing
- **Alpha**: Early development versions

---

[1.0.0]: https://github.com/NortonTech3D/OmniLogger/releases/tag/v1.0.0
