# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-04

### Added

#### Core Features
- Initial release of OmniLogger data logger system
- Support for Lolin (WEMOS) ESP32-S2 Mini microcontroller
- Multi-sensor support architecture (up to 8 sensors)
- SD card data logging with CSV format
- Web-based configuration and monitoring interface
- NTP time synchronization
- Battery voltage monitoring
- Deep sleep power management

#### Sensor Support
- BME280 environmental sensor (I2C)
  - Temperature measurement
  - Humidity measurement
  - Barometric pressure measurement
- DHT22 temperature and humidity sensor
- DS18B20 OneWire temperature sensor
- Generic analog sensor support (ADC-based)

#### Web Interface
- Responsive dashboard with real-time statistics
  - Data point counter
  - Battery voltage display
  - Storage usage tracking
  - SD card health monitoring
  - Active sensor count
  - System uptime
  - Current sensor readings
- Sensor configuration page
  - Enable/disable individual sensors
  - Configure sensor types
  - Set GPIO pins for digital sensors
  - Custom sensor naming
- Settings page
  - WiFi configuration (SSID and password)
  - Measurement interval adjustment
  - Deep sleep mode toggle
  - Timezone offset configuration
  - Device reboot function
- Data management page
  - File listing
  - File download functionality

#### Data Logging
- Automatic CSV file creation with date-based naming
- Configurable measurement intervals
- Timestamp for all data points
- Dynamic CSV headers based on sensor configuration
- Daily file rotation
- Storage space monitoring

#### WiFi Features
- Access Point (AP) mode for initial configuration
  - Default SSID: "OmniLogger"
  - Default password: "omnilogger123"
- Station (STA) mode for network connectivity
- Automatic fallback to AP mode if connection fails
- Configurable WiFi credentials via web interface

#### Power Management
- Battery voltage monitoring via ADC
- Deep sleep mode for battery operation
- Configurable sleep intervals
- Automatic wake and measurement cycles
- USB power detection

#### Configuration System
- Persistent configuration storage using NVS (Preferences)
- Web-based configuration interface
- Default values for first-time setup
- Configuration import/export capability

#### Documentation
- Comprehensive README with setup instructions
- Quick Start Guide for rapid deployment
- Detailed wiring diagrams and pin configurations
- Example configurations for common use cases
- Troubleshooting guide with solutions
- Contributing guidelines for developers
- MIT License

### Technical Details

#### Hardware Support
- ESP32-S2 based boards (Lolin S2 Mini)
- I2C sensors on configurable pins (default SDA: GPIO33, SCL: GPIO35)
- SPI SD card interface (default CS: GPIO12)
- ADC inputs for analog sensors (GPIO1-10)
- Digital sensor inputs on any available GPIO

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
