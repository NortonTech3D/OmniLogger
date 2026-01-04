# OmniLogger

[![License: AGPL-3.0](https://img.shields.io/badge/License-AGPL%203.0-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

A semi-universal data logger system using the Lolin (WEMOS) ESP-32 S2 Mini as the backbone. Production-ready firmware for environmental monitoring, sensor data collection, and IoT applications.

## Features

- **Multi-Sensor Support**: Supports various sensor types including:
  - BME280 (Temperature, Humidity, Pressure via I2C)
  - DHT22 (Temperature, Humidity)
  - DS18B20 (Temperature via OneWire)
  - Analog sensors (ADC-based)
  - Support for up to 8 sensors simultaneously

- **Web Interface**: Complete web-based dashboard for:
  - Real-time system status monitoring
  - Battery voltage and health monitoring
  - SD card health and storage statistics
  - Data point counting
  - Sensor configuration and management
  - System settings (WiFi, measurement intervals, etc.)
  - Data file browsing and downloading

- **Data Logging**: 
  - CSV-based data logging to SD card
  - Automatic file rotation by date
  - Timestamped data points
  - Configurable measurement intervals
  - Optional in-memory buffering with periodic flush to reduce SD card wear

- **Time Synchronization**:
  - Automatic NTP time synchronization
  - Configurable timezone offset
  - Accurate timestamps for all measurements

- **Power Management**:
  - Battery voltage monitoring
  - Optional deep sleep mode for battery operation
  - Configurable wake intervals

- **WiFi Connectivity**:
  - Station (STA) mode for connecting to existing WiFi
  - Access Point (AP) mode for standalone operation
  - Configurable AP SSID and password via web interface
  - Automatic fallback to AP mode if WiFi connection fails

## Hardware Requirements

### Required Components
- **Lolin (WEMOS) ESP32-S2 Mini** development board
- **MicroSD Card Module** (SPI interface)
- **MicroSD Card** (any size, formatted as FAT32)
- **Sensors** (one or more):
  - BME280 sensor (I2C)
  - DHT22 sensor
  - DS18B20 temperature sensor
  - Analog sensors
- **Battery** (optional, for portable operation)
  - 3.7V LiPo battery with voltage divider for monitoring

### Pin Configuration

Default pin assignments for ESP32-S2 Mini:

| Function | GPIO Pin | Notes |
|----------|----------|-------|
| SD Card CS | GPIO 12 | SPI Chip Select |
| SD Card SCK | GPIO 7 | SPI Clock (default) |
| SD Card MISO | GPIO 9 | SPI MISO (default) |
| SD Card MOSI | GPIO 11 | SPI MOSI (default) |
| I2C SDA | GPIO 33 | For BME280 and other I2C sensors |
| I2C SCL | GPIO 35 | For BME280 and other I2C sensors |
| Battery ADC | GPIO 1 | Analog input for battery monitoring |

Digital sensors (DHT22, DS18B20) can be connected to any available GPIO pin and configured via the web interface.

### Wiring Diagram

```
ESP32-S2 Mini                SD Card Module
    3V3 -------------------- VCC
    GND -------------------- GND
    GPIO 12 ---------------- CS
    GPIO 7 ----------------- SCK
    GPIO 9 ----------------- MISO
    GPIO 11 ---------------- MOSI

ESP32-S2 Mini                BME280 (I2C)
    3V3 -------------------- VCC
    GND -------------------- GND
    GPIO 33 ---------------- SDA
    GPIO 35 ---------------- SCL

ESP32-S2 Mini                DHT22
    3V3 -------------------- VCC
    GND -------------------- GND
    GPIO [configurable] ---- DATA

ESP32-S2 Mini                DS18B20
    3V3 -------------------- VCC
    GND -------------------- GND
    GPIO [configurable] ---- DATA (with 4.7kΩ pull-up resistor)

Battery Voltage Monitoring (2:1 voltage divider):
    Battery+ --- [10kΩ] --- GPIO 1 --- [10kΩ] --- GND
```

## Software Setup

### Prerequisites
- [PlatformIO](https://platformio.org/) installed (or Arduino IDE with ESP32 support)
- USB cable for programming

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/NortonTech3D/OmniLogger.git
   cd OmniLogger
   ```

2. **Install dependencies** (automatic with PlatformIO):
   The `platformio.ini` file contains all required libraries:
   - Adafruit BME280 Library
   - Adafruit Unified Sensor
   - DHT sensor library
   - OneWire
   - DallasTemperature
   - ArduinoJson

3. **Build and upload**:
   ```bash
   # Using PlatformIO CLI
   pio run --target upload
   
   # Or using PlatformIO IDE
   # Click the Upload button in VS Code or your IDE
   ```

4. **Monitor serial output** (optional):
   ```bash
   pio device monitor
   ```

### First-Time Setup

1. **Power on the device**: After uploading the firmware, the ESP32-S2 will boot up.

2. **Connect to WiFi AP**: 
   - On first boot, the device creates a WiFi Access Point
   - SSID: `OmniLogger`
   - Password: `omnilogger123`

3. **Access web interface**:
   - Connect to the OmniLogger WiFi network
   - Open a browser and navigate to: `http://192.168.4.1`
   - You should see the OmniLogger dashboard

4. **Configure WiFi** (optional):
   - Go to the "Settings" tab
   - Enter your WiFi SSID and password
   - Save settings and reboot
   - The device will connect to your WiFi network
   - Find the new IP address in your router's DHCP client list, or check serial monitor

5. **Configure sensors**:
   - Go to the "Sensors" tab
   - Enable desired sensors
   - Select sensor types from dropdown
   - For digital sensors (DHT22, DS18B20), specify the GPIO pin
   - Name each sensor for easy identification
   - Save configuration and reboot

6. **Adjust settings**:
   - Set measurement interval (default: 60 seconds)
   - Configure timezone offset
   - Enable/disable deep sleep mode
   - Configure WiFi AP settings (SSID and password)
   - Enable/disable data buffering
   - Set buffer flush interval (if buffering enabled)
   - Save settings

## Advanced Features

### WiFi Access Point Configuration

You can now customize the WiFi Access Point settings through the web interface:

1. Navigate to the **Settings** tab
2. Scroll to **WiFi Access Point Configuration**
3. Enter your desired AP SSID and password (minimum 8 characters)
4. Click **Save Settings**
5. Reboot the device for changes to take effect

This is useful when you want to:
- Use a custom network name for your OmniLogger
- Set a stronger password for security
- Distinguish between multiple OmniLogger devices

### Data Buffering

Data buffering is an optional feature that stores sensor readings in the ESP-32's RAM before writing them to the SD card. This provides several benefits:

**Benefits:**
- **Reduced SD Card Wear**: Fewer write operations extend SD card lifespan
- **Lower Power Consumption**: Batch writes are more efficient than individual writes
- **Better Performance**: Measurements complete faster without waiting for SD writes

**How to use:**
1. Navigate to the **Settings** tab
2. Check **Enable Data Buffering**
3. Set the **Flush Interval** (default: 300 seconds / 5 minutes)
4. Click **Save Settings**
5. Reboot if needed

**Buffer Capacity:**
- The buffer can hold up to 100 data points
- When full, data is automatically flushed to the SD card
- Buffer status is shown on the Dashboard (e.g., "25 / 100")

**Important Notes:**
- Buffered data is lost if power is interrupted before a flush
- For critical applications, keep buffering disabled or use short flush intervals
- Deep sleep mode automatically flushes the buffer before sleeping

## Usage

### Dashboard Tab
- View real-time system statistics:
  - Total data points logged
  - Battery voltage
  - Storage usage
  - SD card health status
  - Active sensor count
  - System uptime
  - Buffer status (when buffering is enabled)
- Current sensor readings displayed in real-time

### Sensors Tab
- Configure up to 8 sensors
- Supported sensor types:
  - **BME280**: I2C temperature, humidity, and pressure sensor
  - **DHT22**: Digital temperature and humidity sensor
  - **DS18B20**: OneWire temperature sensor
  - **Analog**: Generic analog input (0-3.3V)
- Enable/disable individual sensors
- Assign custom names
- Configure GPIO pins for digital sensors

### Settings Tab
- **WiFi Station Configuration**: Set SSID and password for connecting to existing WiFi
- **WiFi Access Point Configuration**: Configure AP SSID and password for standalone mode
- **Data Buffering**: Enable optional data buffering with configurable flush interval
  - Store data in ESP-32 memory before writing to SD card
  - Reduces SD card wear and power consumption
  - Configurable flush interval (default: 5 minutes)
- **Measurement Interval**: Set how often sensors are read (in seconds)
- **Deep Sleep Mode**: Enable for battery-powered operation
- **Timezone Offset**: Configure timezone for accurate timestamps
- **Reboot Device**: Restart the ESP32-S2

### Data Tab
- Browse logged data files
- Download CSV files directly from the web interface
- Files are organized by date: `data_YYYYMMDD.csv`

## Data Format

Data is logged in CSV format with the following structure:

```csv
Timestamp,Environment_Temp_C,Environment_Humidity_%,Environment_Pressure_hPa
2026-01-04 12:00:00,22.50,45.30,1013.25
2026-01-04 12:01:00,22.48,45.35,1013.22
```

The CSV header is dynamically generated based on configured sensors.

## Power Consumption

- **Active mode** (WiFi on, sensors reading): ~80-150mA
- **Deep sleep mode** (between measurements): ~20-50µA
- **Battery life** (with 2000mAh battery):
  - Continuous operation: ~15-20 hours
  - Deep sleep (1 measurement/minute): Several days to weeks
  - Deep sleep (1 measurement/5 minutes): Weeks to months

## Security Considerations

OmniLogger includes several security features to protect your data and device:

### Built-in Security Features
- **Input Validation**: All user inputs are validated before processing
- **Path Traversal Protection**: File download requests are checked to prevent unauthorized access
- **JSON Validation**: API endpoints validate JSON structure before parsing
- **Bounds Checking**: Array accesses and sensor readings are validated
- **Password Requirements**: AP passwords must be 8+ characters (WPA2 requirement)
- **Safe String Handling**: All string operations use bounded functions to prevent overflows

### Best Practices for Deployment
1. **Change Default AP Password**: The default AP password is "omnilogger123" - change this immediately
2. **Use Strong WiFi Credentials**: If connecting to WiFi, use a strong password
3. **Network Security**: Consider using a separate network/VLAN for IoT devices
4. **Physical Security**: Protect the device physically as it stores WiFi credentials
5. **Regular Updates**: Keep the firmware updated for security patches
6. **Data Privacy**: Sensor data is stored locally on the SD card - secure it appropriately

### Limitations
- **No Encryption**: Web interface traffic is not encrypted (HTTP, not HTTPS)
- **No Authentication**: Web interface does not require login (suitable for local/trusted networks)
- **WiFi Credentials**: Stored in plaintext in ESP32 flash memory

**Recommendation**: Deploy OmniLogger on trusted networks only, or implement additional network-level security measures.

## Troubleshooting

### SD Card Not Detected
- Check wiring connections
- Ensure SD card is formatted as FAT32
- Try a different SD card
- Check if CS pin configuration matches hardware (default GPIO 12)

### Sensor Not Reading
- Verify sensor is properly wired
- Check I2C pull-up resistors for BME280 (usually built-in to modules)
- For DS18B20, ensure 4.7kΩ pull-up resistor is installed
- Verify sensor is enabled in configuration
- Check GPIO pin assignment matches hardware

### Cannot Connect to WiFi
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure WiFi network is 2.4GHz (ESP32-S2 doesn't support 5GHz)
- Check serial monitor for connection status
- Device will fall back to AP mode if connection fails

### Web Interface Not Loading
- Verify you're connected to the correct network
- Try accessing by IP address
- Clear browser cache
- Check if web server started successfully in serial monitor

## Development

### Project Structure
```
OmniLogger/
├── platformio.ini          # PlatformIO configuration
├── src/
│   ├── main.cpp           # Main application entry point
│   ├── config.h           # Configuration management
│   ├── sensors.h          # Sensor management
│   ├── datalogger.h       # SD card data logging
│   └── webserver.h        # Web server and interface
├── LICENSE
└── README.md
```

### Extending Sensor Support

To add a new sensor type:

1. Add sensor type to `SensorType` enum in `config.h`
2. Implement initialization in `SensorManager::begin()` in `sensors.h`
3. Implement reading logic in `SensorManager::readAllSensors()`
4. Update CSV header and data formatting methods
5. Add option to web interface sensor type dropdown

## License

This project is licensed under the GNU Affero General Public License v3.0 - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. See CONTRIBUTING.md for detailed guidelines.

## Support

For issues, questions, or suggestions, please open an issue on the GitHub repository.

## Acknowledgments

- Built for the Lolin (WEMOS) ESP32-S2 Mini
- Uses Arduino framework and PlatformIO
- Libraries from Adafruit and community contributors

## Repository

- **GitHub**: https://github.com/NortonTech3D/OmniLogger
- **Documentation**: See QUICKSTART.md, CONTRIBUTING.md, and TROUBLESHOOTING.md
- **Examples**: See examples/ directory for wiring diagrams and configurations
