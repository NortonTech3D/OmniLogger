# OmniLogger Quick Start Guide

Get your OmniLogger up and running in 5 simple steps!

## What You Need

- ESP32-S2 Mini board
- MicroSD card (formatted as FAT32)
- SD card module
- At least one sensor (BME280 recommended for beginners)
- USB cable
- Breadboard and jumper wires

## Step 1: Wire It Up (5 minutes)

### Minimal Setup - BME280 + SD Card

```
SD Card Module → ESP32-S2 Mini
VCC    → 3V3
GND    → GND
CS     → GPIO 12
SCK    → GPIO 7
MOSI   → GPIO 11
MISO   → GPIO 9

BME280 Sensor → ESP32-S2 Mini
VCC    → 3V3
GND    → GND
SDA    → GPIO 33
SCL    → GPIO 35
```

**Note:** See `examples/WIRING.md` for detailed wiring diagrams.

## Step 2: Upload Firmware (5 minutes)

### Using PlatformIO (Recommended)

1. Install PlatformIO:
   ```bash
   pip install platformio
   ```

2. Clone and upload:
   ```bash
   git clone https://github.com/NortonTech3D/OmniLogger.git
   cd OmniLogger
   pio run --target upload
   ```

### Using Arduino IDE

1. Install ESP32 board support
2. Install required libraries (see `platformio.ini`)
3. Open `src/main.cpp`
4. Select "ESP32S2 Dev Module" board
5. Upload

## Step 3: Connect to WiFi (2 minutes)

1. Power on your ESP32-S2
2. On your phone/computer, connect to WiFi:
   - **SSID:** `OmniLogger`
   - **Password:** `omnilogger123`
3. Open browser and go to: `http://192.168.4.1`

## Step 4: Configure (5 minutes)

### First-Time Setup

1. **Configure WiFi** (optional but recommended):
   - Go to "Settings" tab
   - Enter your WiFi SSID and password
   - Click "Save Settings"
   - Click "Reboot Device"
   - Find new IP in your router or serial monitor

2. **Configure Sensors**:
   - Go to "Sensors" tab
   - Sensor 1 should show BME280 (auto-detected)
   - Enable it if not already
   - Name it (e.g., "Environment")
   - Click "Save Sensor Configuration"
   - Reboot if changes made

3. **Adjust Settings**:
   - Go to "Settings" tab
   - Set measurement interval (default: 60 seconds)
   - Set timezone offset (e.g., -5 for EST)
   - Enable deep sleep if battery powered
   - Click "Save Settings"

## Step 5: Start Logging! (instant)

1. Go to "Dashboard" tab
2. You should see:
   - Real-time sensor readings
   - Data point count increasing
   - Battery voltage
   - SD card status
   - Storage used

3. Data is automatically logged to SD card in CSV format

## Viewing Your Data

### Web Interface
- Go to "Data" tab
- Click "Refresh" to see files
- Click "Download" on any file
- Open in Excel, Google Sheets, or any CSV viewer

### Direct from SD Card
- Power off device
- Remove SD card
- Insert in computer
- Files are named `data_YYYYMMDD.csv`

## Typical Data Output

```csv
Timestamp,Environment_Temp_C,Environment_Humidity_%,Environment_Pressure_hPa
2026-01-04 12:00:00,22.5,45.3,1013.25
2026-01-04 12:01:00,22.6,45.1,1013.22
2026-01-04 12:02:00,22.5,45.4,1013.20
```

## Next Steps

### Add More Sensors

1. Wire up additional sensor (see WIRING.md)
2. Go to "Sensors" tab in web interface
3. Enable sensor slot
4. Select sensor type
5. Set GPIO pin (for digital sensors)
6. Name the sensor
7. Save and reboot

### Battery Operation

1. Add voltage divider for battery monitoring
2. Connect battery (3.7V LiPo recommended)
3. Enable deep sleep in settings
4. Disconnect USB
5. Device will wake, measure, log, and sleep

### Customize

- Change measurement interval for your needs
- Set up timezone for accurate timestamps
- Configure WiFi for remote access
- Add custom sensor names

## Common Issues

### SD Card Not Detected
- Ensure formatted as FAT32
- Check wiring (all 6 connections)
- Try different card

### Sensor Not Working
- Verify wiring matches diagram
- Check sensor type in configuration
- Ensure pull-up resistors for DHT22/DS18B20

### Can't Access Web Interface
- Verify connected to OmniLogger WiFi
- Try http://192.168.4.1 (include http://)
- Clear browser cache

**See `TROUBLESHOOTING.md` for detailed solutions.**

## Default Values

| Setting | Default Value |
|---------|--------------|
| AP SSID | OmniLogger |
| AP Password | omnilogger123 |
| Measurement Interval | 60 seconds |
| Timezone Offset | 0 (UTC) |
| Deep Sleep | Disabled |
| SD Card CS Pin | GPIO 12 |
| I2C SDA | GPIO 33 |
| I2C SCL | GPIO 35 |

## Tips for Success

1. **Start Simple**: Get basic logging working before adding complexity
2. **Check Serial Monitor**: Provides detailed debug information
3. **Format SD Card**: Always use FAT32, not exFAT or NTFS
4. **Stable Power**: Use quality USB power supply (1A minimum)
5. **Test Sensors**: Verify each sensor individually before combining
6. **Backup Data**: Download important data files regularly
7. **Monitor Battery**: Check voltage regularly for battery operation

## Support

- **Documentation**: See README.md for full documentation
- **Examples**: Check examples/ directory for configurations
- **Troubleshooting**: See TROUBLESHOOTING.md
- **Issues**: Open issue on GitHub
- **Community**: Arduino and ESP32 forums

## Pin Quick Reference

```
ESP32-S2 Mini Pin Assignments

Power:
- 3V3: Power output (max 700mA)
- 5V: USB power in
- GND: Ground

SPI (SD Card):
- GPIO 7: SCK
- GPIO 9: MISO
- GPIO 11: MOSI
- GPIO 12: CS (default, configurable)

I2C (BME280):
- GPIO 33: SDA
- GPIO 35: SCL

ADC (Analog sensors, battery):
- GPIO 1-10: Analog input (0-3.3V)

Digital Sensors:
- Any available GPIO (recommend 1-6, 11-18)
```

## Example Measurement Intervals

| Use Case | Interval | Battery Life (2000mAh) |
|----------|----------|----------------------|
| Real-time monitoring | 5 seconds | ~12 hours |
| Weather station | 5 minutes | ~2 weeks |
| Environmental log | 15 minutes | ~1 month |
| Long-term study | 1 hour | ~6 months |

*Battery life with deep sleep enabled, single BME280 sensor*

## Resources

- **Full Documentation**: README.md
- **Wiring Diagrams**: examples/WIRING.md
- **Configuration Examples**: examples/CONFIGURATIONS.md
- **Troubleshooting**: TROUBLESHOOTING.md
- **GitHub**: https://github.com/NortonTech3D/OmniLogger

---

**Happy Logging! 📊**

For questions or issues, check the documentation or open a GitHub issue.
