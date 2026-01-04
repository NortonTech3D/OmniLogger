# Contributing to OmniLogger

Thank you for your interest in contributing to OmniLogger! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and inclusive
- Provide constructive feedback
- Focus on the technical merit of contributions
- Help others learn and grow

## How to Contribute

### Reporting Bugs

1. Check existing issues to avoid duplicates
2. Use the issue template
3. Include:
   - Detailed description
   - Steps to reproduce
   - Expected vs actual behavior
   - Serial monitor output
   - Hardware configuration
   - Software version

### Suggesting Enhancements

1. Check existing issues and pull requests
2. Clearly describe the enhancement
3. Explain why it would be useful
4. Provide examples if possible

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly
5. Commit with clear messages
6. Push to your fork
7. Open a pull request

## Development Setup

### Prerequisites

- PlatformIO or Arduino IDE
- ESP32-S2 board
- Test hardware (sensors, SD card)
- Serial monitor access

### Local Setup

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/OmniLogger.git
cd OmniLogger

# Install dependencies
pio lib install

# Build
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Code Style Guidelines

### C++ Code Style

```cpp
// Use descriptive variable names
int sensorReading;  // Good
int sr;             // Bad

// Constants in UPPER_CASE
const int MAX_SENSORS = 8;

// Functions and methods in camelCase
void readSensorData() {
    // Function body
}

// Classes in PascalCase
class SensorManager {
    // Class definition
};

// Comment complex logic
// Calculate voltage from ADC reading using voltage divider ratio
float voltage = (rawValue / adcMax) * adcVoltage * voltageDivider;

// Use consistent indentation (2 or 4 spaces, not tabs)
if (condition) {
  doSomething();
  doAnotherThing();
}
```

### File Organization

- Header files (`.h`) for declarations
- Implementation in `.cpp` or inline in headers for templates
- One class per file when possible
- Keep files focused and manageable (< 500 lines ideal)

### Comments

```cpp
// Single-line comments for brief explanations

/*
 * Multi-line comments for:
 * - File headers
 * - Class descriptions
 * - Complex algorithms
 */

/**
 * Documentation comments for public APIs
 * @param value The input value
 * @return The processed result
 */
```

## Architecture

### Project Structure

```
OmniLogger/
├── platformio.ini          # Build configuration
├── src/
│   ├── main.cpp           # Application entry point
│   ├── config.h           # Configuration management
│   ├── sensors.h          # Sensor interface and implementations
│   ├── datalogger.h       # SD card logging
│   └── webserver.h        # Web server and UI
├── examples/              # Example configurations
├── docs/                  # Additional documentation
└── test/                  # Unit tests (future)
```

### Key Components

1. **Config**: Persistent storage using Preferences (NVS)
2. **SensorManager**: Modular sensor interface
3. **DataLogger**: SD card operations
4. **WebServerManager**: HTTP server and REST API

### Design Principles

- **Modularity**: Each component has clear responsibility
- **Configurability**: User-configurable through web interface
- **Extensibility**: Easy to add new sensor types
- **Reliability**: Graceful error handling
- **Efficiency**: Optimized for low power consumption

## Adding New Sensor Types

To add support for a new sensor:

### 1. Define Sensor Type

Edit `src/config.h`:

```cpp
enum SensorType {
  SENSOR_NONE = 0,
  SENSOR_BME280 = 1,
  SENSOR_DHT22 = 2,
  SENSOR_DS18B20 = 3,
  SENSOR_ANALOG = 4,
  SENSOR_YOUR_NEW_SENSOR = 5  // Add here
};
```

### 2. Implement Sensor Reading

Edit `src/sensors.h`:

```cpp
// Add initialization method
void initYourSensor(int index, const SensorConfig& config) {
  Serial.printf("Initializing YourSensor on pin %d...\n", config.pin);
  
  // Initialize sensor hardware
  // ...
  
  sensorTypes[index] = SENSOR_YOUR_NEW_SENSOR;
  strcpy(sensorNames[index], config.name);
}

// Add reading method
void readYourSensor(int index) {
  // Read sensor data
  // ...
  
  readings[index].temperature = temp;  // or other fields
  readings[index].valid = true;
}

// Update begin() method
void begin(const Config& config) {
  // ...
  switch (config.sensors[i].type) {
    // ...
    case SENSOR_YOUR_NEW_SENSOR:
      initYourSensor(i, config.sensors[i]);
      break;
  }
}

// Update readAllSensors() method
void readAllSensors() {
  // ...
  switch (sensorTypes[i]) {
    // ...
    case SENSOR_YOUR_NEW_SENSOR:
      readYourSensor(i);
      break;
  }
}
```

### 3. Update CSV Output

Edit `src/sensors.h`:

```cpp
// Update getCSVData()
String getCSVData(const char* timestamp) const {
  // ...
  switch (sensorTypes[i]) {
    // ...
    case SENSOR_YOUR_NEW_SENSOR:
      csv += "," + String(readings[i].yourValue, 2);
      break;
  }
}

// Update getCSVHeader()
String getCSVHeader() const {
  // ...
  switch (sensorTypes[i]) {
    // ...
    case SENSOR_YOUR_NEW_SENSOR:
      header += "," + prefix + "_YourValue";
      break;
  }
}
```

### 4. Update Web Interface

Edit `src/webserver.h` in the JavaScript section:

```javascript
// Add option to sensor type dropdown
html += '<option value="5"' + (sensor.type === 5 ? ' selected' : '') + '>Your New Sensor</option>';
```

### 5. Update Documentation

- Add sensor to README.md
- Add wiring diagram to examples/WIRING.md
- Add example configuration to examples/CONFIGURATIONS.md
- Update library dependencies in platformio.ini if needed

### 6. Test Thoroughly

- Test sensor detection
- Verify readings are correct
- Check CSV output format
- Test web interface configuration
- Verify data logging
- Test error conditions

## Testing

### Manual Testing Checklist

- [ ] Code compiles without warnings
- [ ] Boots successfully
- [ ] SD card detected
- [ ] Sensors initialize correctly
- [ ] Web interface accessible
- [ ] Settings save and persist
- [ ] Data logging works
- [ ] CSV format correct
- [ ] WiFi connection stable
- [ ] Battery monitoring accurate
- [ ] Deep sleep functions properly

### Serial Monitor Output

Always check serial output during testing:

```
OmniLogger - Data Logger Starting...
========================================
Configuration loaded successfully
LittleFS mounted successfully
SD card initialized successfully
I2C initialized on SDA:33, SCL:35
Initializing BME280 sensor 0...
BME280 initialized successfully
Initialized 1 sensors
Connecting to WiFi: YourNetwork
WiFi connected!
IP address: 192.168.1.100
Time synchronized: 2026-01-04 12:00:00
Web server started
Setup complete!
========================================
```

## Documentation

### Code Documentation

- Document public APIs
- Explain non-obvious code
- Include usage examples
- Document assumptions and limitations

### User Documentation

- Update README.md for user-facing changes
- Add examples for new features
- Update troubleshooting guide
- Keep quick start guide current

## Git Commit Messages

### Format

```
<type>: <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style (formatting, etc.)
- `refactor`: Code restructuring
- `perf`: Performance improvement
- `test`: Adding tests
- `chore`: Maintenance tasks

### Examples

```
feat: Add support for SHT31 temperature sensor

Implements SHT31 sensor type with I2C communication.
Includes initialization, reading, and CSV output.

Closes #123
```

```
fix: Correct battery voltage calculation

The voltage divider ratio was incorrect, causing
battery voltage to read twice the actual value.

Fixes #456
```

```
docs: Update wiring diagram for DHT22 sensor

Added pull-up resistor to diagram and clarified
pin numbering.
```

## Release Process

1. Update version number
2. Update CHANGELOG.md
3. Tag release
4. Create GitHub release
5. Build and test release
6. Announce release

## Questions?

- Open an issue for discussion
- Check existing documentation
- Ask in pull request comments

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes
- Project documentation

Thank you for contributing to OmniLogger!
