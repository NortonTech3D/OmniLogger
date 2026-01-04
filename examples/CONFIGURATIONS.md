# Example Sensor Configurations

This document provides example configurations for different sensor setups.

## Example 1: Weather Station (BME280 only)

**Hardware:**
- ESP32-S2 Mini
- BME280 sensor (I2C)
- SD card module
- 3.7V LiPo battery

**Configuration:**
- Sensor 1: BME280 (I2C address 0x76 or 0x77)
  - Name: "Weather"
  - Type: BME280
  - Enabled: Yes

**Settings:**
- Measurement Interval: 300 seconds (5 minutes)
- Deep Sleep: Enabled
- Timezone: Adjust to your location

**Expected CSV Output:**
```csv
Timestamp,Weather_Temp_C,Weather_Humidity_%,Weather_Pressure_hPa
2026-01-04 12:00:00,22.5,45.3,1013.25
2026-01-04 12:05:00,22.6,45.1,1013.22
```

---

## Example 2: Multi-Sensor Environment Monitor

**Hardware:**
- ESP32-S2 Mini
- BME280 sensor (I2C)
- DHT22 sensor (GPIO 5)
- DS18B20 sensor (GPIO 6)
- SD card module

**Configuration:**
- Sensor 1: BME280
  - Name: "Indoor"
  - Type: BME280
  - Enabled: Yes

- Sensor 2: DHT22
  - Name: "Outdoor"
  - Type: DHT22
  - Pin: GPIO 5
  - Enabled: Yes

- Sensor 3: DS18B20
  - Name: "Probe"
  - Type: DS18B20
  - Pin: GPIO 6
  - Enabled: Yes

**Settings:**
- Measurement Interval: 60 seconds
- Deep Sleep: Disabled (continuous monitoring)

**Expected CSV Output:**
```csv
Timestamp,Indoor_Temp_C,Indoor_Humidity_%,Indoor_Pressure_hPa,Outdoor_Temp_C,Outdoor_Humidity_%,Probe_Temp_C
2026-01-04 12:00:00,22.5,45.3,1013.25,18.2,65.5,25.8
```

---

## Example 3: Analog Sensor Array

**Hardware:**
- ESP32-S2 Mini
- 4x Analog sensors (e.g., soil moisture, light sensors)
- SD card module

**Configuration:**
- Sensor 1: Analog
  - Name: "Soil1"
  - Type: Analog
  - Pin: GPIO 2
  - Enabled: Yes

- Sensor 2: Analog
  - Name: "Soil2"
  - Type: Analog
  - Pin: GPIO 3
  - Enabled: Yes

- Sensor 3: Analog
  - Name: "Light1"
  - Type: Analog
  - Pin: GPIO 4
  - Enabled: Yes

- Sensor 4: Analog
  - Name: "Light2"
  - Type: Analog
  - Pin: GPIO 5
  - Enabled: Yes

**Settings:**
- Measurement Interval: 120 seconds (2 minutes)
- Deep Sleep: Enabled

**Expected CSV Output:**
```csv
Timestamp,Soil1_Value,Soil2_Value,Light1_Value,Light2_Value
2026-01-04 12:00:00,2.45,2.38,1.65,1.72
```

---

## Example 4: Battery-Powered Remote Logger

**Hardware:**
- ESP32-S2 Mini
- BME280 sensor (I2C)
- SD card module
- 3.7V 2000mAh LiPo battery with voltage divider

**Configuration:**
- Sensor 1: BME280
  - Name: "Remote"
  - Type: BME280
  - Enabled: Yes

**Settings:**
- Measurement Interval: 600 seconds (10 minutes)
- Deep Sleep: Enabled
- WiFi: Disabled (or only for configuration)

**Power Optimization:**
- Set measurement interval to maximum acceptable value
- Enable deep sleep mode
- Disable WiFi after initial configuration
- Expected battery life: 2-4 weeks with 10-minute intervals

---

## Pin Usage Reference

### ESP32-S2 Mini Available GPIO Pins

Safe to use for sensors:
- GPIO 1-6 (ADC1)
- GPIO 7-10 (Used by SD card if SPI is default)
- GPIO 11-14
- GPIO 15-18
- GPIO 33-40

**Note:** Some pins have special functions:
- GPIO 0: Boot button (can be used but avoid during boot)
- GPIO 19-20: USB (do not use)
- GPIO 26-32: Not available on S2 Mini
- GPIO 43-46: Reserved

### I2C Sensors (BME280)
- Can share the same I2C bus
- Multiple BME280 sensors require different addresses (0x76 and 0x77)
- Default pins: SDA=GPIO33, SCL=GPIO35

### Digital Sensors (DHT22, DS18B20)
- Each requires its own GPIO pin
- DS18B20 needs 4.7kΩ pull-up resistor
- Can be on any available GPIO

### Analog Sensors
- ESP32-S2 has ADC1 (GPIO1-10) 
- 12-bit resolution (0-4095)
- Input voltage: 0-3.3V
- Use voltage divider for higher voltages

---

## Calibration Notes

### Temperature Sensors
- BME280, DHT22, and DS18B20 typically don't need calibration
- If multiple temperature sensors show different values, this is normal
- BME280 tends to read slightly higher due to self-heating

### Humidity Sensors
- BME280 and DHT22 may drift over time
- Consider periodic calibration using salt solutions
- Keep sensors away from direct airflow for accurate readings

### Analog Sensors
- Calibrate in code based on known reference values
- Example for soil moisture: 
  - Dry air: ~3.3V (4095)
  - In water: ~1.5V (1860)
- Store calibration values and convert in post-processing

---

## Data File Management

### File Naming
Files are automatically named by date: `data_YYYYMMDD.csv`

Example:
- `data_20260104.csv` - Data for January 4, 2026
- `data_20260105.csv` - Data for January 5, 2026

### File Rotation
- New file created automatically each day at midnight
- Previous days' files remain on SD card
- Download or clear old files periodically

### Storage Considerations
- 1 measurement = ~50-100 bytes (depending on sensor count)
- 1440 measurements/day (1 per minute) = ~70-140 KB/day
- 1 GB SD card = ~20-30 years of data at 1-minute intervals
- 32 GB SD card = plenty of space for any practical application

### Data Backup
1. Download files via web interface
2. Copy to computer for analysis
3. Import into Excel, Python, R, etc.
4. Delete old files from SD card to free space
