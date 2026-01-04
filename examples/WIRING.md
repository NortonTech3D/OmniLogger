# Detailed Wiring Guide

This guide provides detailed wiring instructions for connecting sensors and modules to the ESP32-S2 Mini.

## Table of Contents
1. [SD Card Module](#sd-card-module)
2. [BME280 Sensor (I2C)](#bme280-sensor-i2c)
3. [DHT22 Sensor](#dht22-sensor)
4. [DS18B20 Temperature Sensor](#ds18b20-temperature-sensor)
5. [Analog Sensors](#analog-sensors)
6. [Battery Connection](#battery-connection)
7. [Complete Wiring Example](#complete-wiring-example)

---

## SD Card Module

The SD card module uses SPI communication and is essential for data logging.

### Connections:

| SD Card Module Pin | ESP32-S2 Mini Pin | Notes |
|-------------------|-------------------|-------|
| VCC | 3V3 | Power supply |
| GND | GND | Ground |
| CS (Chip Select) | GPIO 12 | Configurable in software |
| SCK (Clock) | GPIO 7 | SPI Clock |
| MOSI (Master Out) | GPIO 11 | SPI Data Out |
| MISO (Master In) | GPIO 9 | SPI Data In |

### Notes:
- Ensure SD card is formatted as FAT32
- Module typically includes voltage level shifters for 5V compatibility
- Some modules have onboard 3.3V regulation
- CS pin can be changed in software (default GPIO 12)

### SD Card Slot Pinout:
```
     SD Card (looking at contacts)
     ┌─────────────┐
     │ 9 1 2 3 4 5 6 7 8 │
     └─────────────┘
     
Pin 1: CS (Chip Select)
Pin 2: MOSI (Data In)
Pin 3: GND
Pin 4: VCC (3.3V)
Pin 5: SCK (Clock)
Pin 6: GND
Pin 7: MISO (Data Out)
Pin 8: Not used
Pin 9: Not used
```

---

## BME280 Sensor (I2C)

BME280 measures temperature, humidity, and barometric pressure via I2C.

### Connections:

| BME280 Pin | ESP32-S2 Mini Pin | Notes |
|-----------|-------------------|-------|
| VCC/VIN | 3V3 | Power (3.3V) |
| GND | GND | Ground |
| SDA | GPIO 33 | I2C Data |
| SCL | GPIO 35 | I2C Clock |

### Notes:
- Most BME280 modules have built-in pull-up resistors
- Default I2C address is usually 0x76 or 0x77
- Can share I2C bus with other I2C sensors
- Software will auto-detect address

### Multiple BME280 Sensors:
- Two BME280 sensors can share the same I2C bus
- One must be configured for 0x76, the other for 0x77
- Check module documentation for address selection (usually a solder jumper)

---

## DHT22 Sensor

DHT22 measures temperature and humidity using a single-wire protocol.

### Connections:

| DHT22 Pin | ESP32-S2 Mini Pin | Notes |
|----------|-------------------|-------|
| VCC (Pin 1) | 3V3 or 5V | Power supply |
| DATA (Pin 2) | GPIO (configurable) | Data line |
| NC (Pin 3) | Not connected | Not used |
| GND (Pin 4) | GND | Ground |

### DHT22 Pinout (facing sensor):
```
  ┌─────────────┐
  │ ┌─┐     ┌─┐ │
  │ │ │     │ │ │
  │ │ │     │ │ │
  └─┴─┴─┴─┴─┴─┴─┘
    1 2 3   4
    
1: VCC (3.3V or 5V)
2: DATA
3: Not connected
4: GND
```

### Notes:
- Add 4.7kΩ - 10kΩ pull-up resistor between DATA and VCC
- Some modules include built-in pull-up resistor
- Can use any available GPIO pin (configure in web interface)
- Recommended pins: GPIO 1-6, 11-18
- Keep wires short (< 20m for reliable operation)

### With Built-in Pull-up:
Most DHT22 modules have 3 pins and include the pull-up resistor:
- Connect VCC, DATA, and GND only
- No external resistor needed

---

## DS18B20 Temperature Sensor

DS18B20 is a digital temperature sensor using the OneWire protocol.

### Connections:

| DS18B20 Pin | ESP32-S2 Mini Pin | Notes |
|------------|-------------------|-------|
| VCC (Red) | 3V3 | Power supply |
| DATA (Yellow) | GPIO (configurable) | Data line |
| GND (Black) | GND | Ground |

### DS18B20 Pinout (TO-92 package, flat side facing you):
```
   ┌─────────┐
   │  ┌───┐  │
   │  └───┘  │
   └─────────┘
     1 2 3
     
1: GND (Black)
2: DATA (Yellow)
3: VCC (Red)
```

### Notes:
- **REQUIRED**: 4.7kΩ pull-up resistor between DATA and VCC
- Can use any available GPIO pin
- Supports multiple sensors on same bus (OneWire addressing)
- Can be powered in "parasitic power" mode (only 2 wires) for short distances

### Multiple DS18B20 Sensors:
All sensors share the same 3 wires:
```
ESP32-S2          Pull-up     DS18B20 #1    DS18B20 #2    DS18B20 #3
GPIO X ─────┬─── 4.7kΩ ─── VCC         ┌─── VCC         ┌─── VCC
            │                  │         │       │         │
            ├────────────── DATA        ├─── DATA        ├─── DATA
            │                  │         │       │         │
GND ────────┴────────────── GND         └─── GND         └─── GND
3V3 ────────┬─── 4.7kΩ ────┘
```

Each sensor has a unique 64-bit ROM address for identification.

---

## Analog Sensors

Analog sensors output a voltage that can be read by the ESP32-S2 ADC.

### Connections:

| Sensor Output | ESP32-S2 Mini Pin | Notes |
|--------------|-------------------|-------|
| Signal (0-3.3V) | GPIO 1-10 (ADC1) | Analog input |
| VCC | 3V3 | Power supply |
| GND | GND | Ground |

### Notes:
- ESP32-S2 ADC reads 0-3.3V
- 12-bit resolution: 0-4095 values
- Do NOT exceed 3.3V on ADC pins
- For 5V sensors, use voltage divider

### Voltage Divider for 5V Sensors:

To read 0-5V signals on 0-3.3V ADC:

```
Sensor Output (0-5V)
       │
       ├─── 10kΩ ───┬─── ADC Pin (0-3.3V)
       │            │
       │            ├─── 4.7kΩ ─── GND
       │            │
```

Formula: Vout = Vin × (R2 / (R1 + R2))
- R1 = 10kΩ
- R2 = 4.7kΩ
- 5V input → 1.6V at ADC (safe)
- Adjust in software: actualVoltage = readVoltage × 5.0 / 3.3

### Common Analog Sensors:

1. **Soil Moisture Sensor**
   - VCC: 3.3V
   - GND: GND
   - A0: GPIO (any ADC pin)

2. **Photoresistor (LDR)**
   - Voltage divider circuit:
   ```
   3V3 ─── LDR ───┬─── ADC Pin
                  │
                  ├─── 10kΩ ─── GND
   ```

3. **Potentiometer**
   - Left pin: GND
   - Center pin: ADC Pin
   - Right pin: 3V3

---

## Battery Connection

For portable operation with battery voltage monitoring.

### Connections:

| Battery | Component | ESP32-S2 Pin |
|---------|-----------|-------------|
| Battery + | ─── 10kΩ ───┬─── | GPIO 1 (ADC) |
| | | ├─── 10kΩ ─── | GND |
| Battery - | | GND |
| Battery + (power) | | 5V or 3V3 via regulator |

### Voltage Divider Circuit:
```
Battery + (3.7V - 4.2V for LiPo)
    │
    ├─── 10kΩ ────┬──── GPIO 1 (ADC)
    │             │
    │             ├─── 10kΩ ─── GND
    │
    └──── [LiPo Charger/Protection] ──── ESP32-S2 5V or 3V3
```

### Notes:
- 2:1 voltage divider reduces battery voltage to safe ADC range
- 4.2V (max LiPo) → 2.1V at ADC (safe)
- Software calculates actual battery voltage
- Add LiPo charger/protection board for safety
- ESP32-S2 can be powered via:
  - USB (5V)
  - 5V pin (with voltage regulator)
  - 3V3 pin (direct from LiPo with protection)

### Recommended Battery Setup:
- Use TP4056 charger module with protection
- 3.7V 18650 or LiPo battery
- HT7333 3.3V LDO regulator for stable power
- Battery monitoring via voltage divider

---

## Complete Wiring Example

### Weather Station Configuration
- ESP32-S2 Mini
- SD Card Module
- BME280 Sensor
- Battery with monitoring

```
                                    ESP32-S2 Mini
                            ┌─────────────────────────┐
                            │                         │
    SD Card Module          │  GPIO 7  (SCK)          │
    ┌──────────────┐        │  GPIO 9  (MISO)         │
    │ VCC ─────────────────────── 3V3                 │
    │ GND ─────────────────────── GND                 │
    │ CS  ─────────────────────── GPIO 12             │
    │ SCK ─────────────────────── GPIO 7              │
    │ MOSI ────────────────────── GPIO 11             │
    │ MISO ────────────────────── GPIO 9              │
    └──────────────┘        │                         │
                            │                         │
    BME280 Sensor           │                         │
    ┌──────────────┐        │                         │
    │ VCC ─────────────────────── 3V3                 │
    │ GND ─────────────────────── GND                 │
    │ SDA ─────────────────────── GPIO 33             │
    │ SCL ─────────────────────── GPIO 35             │
    └──────────────┘        │                         │
                            │                         │
    Battery Monitoring      │                         │
    Battery + ─── 10kΩ ──────── GPIO 1              │
                    │       │                         │
                    10kΩ    │                         │
                    │       │                         │
                   GND ────────── GND                 │
                            │                         │
    Power Supply            │                         │
    Battery + ──────────────────── 5V (via regulator) │
    Battery - ──────────────────── GND                │
                            │                         │
                            └─────────────────────────┘
```

### Full Multi-Sensor Setup

All sensors connected simultaneously:

```
ESP32-S2 Mini Connections:

Power:
- 3V3: BME280, SD Card, DHT22, DS18B20, Analog sensors
- GND: All sensors common ground
- 5V: From battery via regulator

SPI (SD Card):
- GPIO 7: SCK
- GPIO 9: MISO
- GPIO 11: MOSI
- GPIO 12: CS

I2C (BME280):
- GPIO 33: SDA
- GPIO 35: SCL

Digital (DHT22):
- GPIO 5: DHT22 Data + 10kΩ pull-up to 3V3

OneWire (DS18B20):
- GPIO 6: DS18B20 Data + 4.7kΩ pull-up to 3V3

Analog:
- GPIO 1: Battery voltage (via divider)
- GPIO 2: Analog sensor 1
- GPIO 3: Analog sensor 2
- GPIO 4: Analog sensor 3
```

---

## Troubleshooting Wiring Issues

### SD Card Not Detected
- Check all 6 connections (VCC, GND, CS, SCK, MOSI, MISO)
- Ensure SD card is formatted as FAT32
- Verify CS pin matches software configuration
- Try different SD card
- Check for loose connections

### BME280 Not Found
- Verify I2C connections (SDA, SCL, VCC, GND)
- Check I2C address (0x76 or 0x77)
- Ensure pull-up resistors present (usually on module)
- Try swapping SDA and SCL (in case of error)

### DHT22 Reading Errors
- Verify pull-up resistor (4.7kΩ - 10kΩ)
- Check VCC (can use 3.3V or 5V)
- Ensure correct pin in software configuration
- Keep wires short
- Try different GPIO pin

### DS18B20 Not Responding
- Check pull-up resistor (4.7kΩ required!)
- Verify pin connections (VCC, DATA, GND)
- Ensure correct pin in software
- Test with single sensor first

### Analog Readings Unstable
- Add 0.1µF capacitor between ADC pin and GND
- Use voltage divider for sensors > 3.3V
- Keep sensor wires away from power lines
- Add delay between readings in code

---

## Safety Notes

1. **Never exceed 3.3V on GPIO pins** - ESP32-S2 is NOT 5V tolerant
2. **Use voltage dividers** for sensors with >3.3V output
3. **LiPo batteries** require protection circuits
4. **Check polarity** before powering up
5. **Start with one sensor** and add more one at a time
6. **Use proper gauge wire** - 22-24 AWG for connections
7. **Secure connections** - use breadboard, perfboard, or solder
8. **Label wires** - especially for complex setups

---

## Parts List

### Minimum Setup:
- ESP32-S2 Mini board
- MicroSD card module
- MicroSD card (any size, FAT32)
- Breadboard and jumper wires
- At least one sensor (BME280 recommended)

### Recommended Additional Parts:
- Assorted resistors (4.7kΩ, 10kΩ)
- Capacitors (0.1µF)
- LiPo battery (2000mAh recommended)
- TP4056 charger module
- HT7333 voltage regulator
- Enclosure (weatherproof if outdoor)

### Tools:
- Multimeter (for testing connections)
- Soldering iron (for permanent assembly)
- Wire strippers
- Heat shrink tubing
- Label maker or tape

---

## Next Steps

After wiring:
1. Double-check all connections against this guide
2. Upload firmware via USB
3. Connect to OmniLogger WiFi
4. Access web interface
5. Configure sensors in web UI
6. Verify sensor readings in dashboard
7. Check SD card logging
8. Enjoy your data logger!
