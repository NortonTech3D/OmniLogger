# Troubleshooting Guide

This guide helps you diagnose and fix common issues with the OmniLogger system.

## Quick Diagnostic Checklist

Before diving into specific problems, check these basics:

- [ ] Power LED is on
- [ ] USB cable is properly connected
- [ ] SD card is inserted and formatted as FAT32
- [ ] WiFi credentials are correct
- [ ] Sensors are properly wired
- [ ] Serial monitor shows boot messages

## Table of Contents

1. [Power and Boot Issues](#power-and-boot-issues)
2. [WiFi Connection Problems](#wifi-connection-problems)
3. [SD Card Issues](#sd-card-issues)
4. [Sensor Problems](#sensor-problems)
5. [Web Interface Issues](#web-interface-issues)
6. [Data Logging Issues](#data-logging-issues)
7. [Battery and Power Management](#battery-and-power-management)
8. [Upload and Compilation Errors](#upload-and-compilation-errors)

---

## Power and Boot Issues

### Device Won't Power On

**Symptoms:**
- No LED lights
- No response when connected to USB
- Device appears dead

**Solutions:**

1. **Check USB cable**
   - Try a different USB cable (data-capable, not charge-only)
   - Test cable with another device
   - Try different USB port or power source

2. **Check power supply**
   - USB port should provide at least 500mA
   - Try USB charger (5V, 1A minimum)
   - Avoid USB hubs without external power

3. **Check for short circuits**
   - Disconnect all sensors and modules
   - Power on with only USB connected
   - If it works, reconnect components one by one

4. **Check ESP32-S2 board**
   - Look for physical damage
   - Check for bent or broken pins
   - Test with minimal configuration (no sensors)

### Device Boots but Crashes

**Symptoms:**
- Boots briefly then resets
- Continuous reboot loop
- Watchdog timer resets

**Solutions:**

1. **Check serial monitor for error messages**
   ```bash
   pio device monitor
   ```

2. **Common crash causes:**
   - Insufficient power (add capacitor near ESP32)
   - Short circuit on sensor connections
   - Corrupted configuration (reset via web interface)
   - Bad SD card

3. **Reset to defaults:**
   - Reflash firmware
   - Clear NVS (flash storage):
     ```bash
     pio run -t erase
     pio run -t upload
     ```

### Slow Boot or Hangs

**Symptoms:**
- Takes more than 30 seconds to boot
- Hangs at specific initialization step

**Solutions:**

1. **Check SD card**
   - Remove SD card and test boot
   - Format SD card as FAT32
   - Try different SD card

2. **Check sensor initialization**
   - Disconnect sensors one by one
   - Identify which sensor causes hang
   - Verify sensor wiring

3. **WiFi connection timeout**
   - Device may wait for WiFi connection
   - Will fall back to AP mode after ~10 seconds
   - This is normal behavior

---

## WiFi Connection Problems

### Cannot Connect to OmniLogger AP

**Symptoms:**
- OmniLogger WiFi network not visible
- Cannot connect to AP

**Solutions:**

1. **Verify AP mode is active**
   - Check serial monitor
   - Should see "Starting Access Point mode..."
   - AP IP should be 192.168.4.1

2. **Check WiFi settings on your device**
   - Enable WiFi on phone/computer
   - Scan for networks
   - Look for "OmniLogger" SSID
   - Default password: "omnilogger123"

3. **Try different WiFi channel**
   - Some devices don't support all channels
   - ESP32-S2 uses 2.4GHz only

4. **Reset WiFi configuration**
   - Reflash firmware
   - Will restore default AP settings

### Cannot Connect to Home WiFi

**Symptoms:**
- OmniLogger won't connect to configured WiFi
- Falls back to AP mode

**Solutions:**

1. **Verify WiFi credentials**
   - SSID is case-sensitive
   - Password is case-sensitive
   - No extra spaces in SSID or password

2. **Check WiFi network compatibility**
   - Must be 2.4GHz network (not 5GHz)
   - WPA2 Personal supported
   - WPA3 and Enterprise not supported
   - Open networks may cause issues

3. **Check signal strength**
   - Move closer to router
   - Remove obstacles
   - Check router is broadcasting SSID

4. **Router issues**
   - Check MAC address filtering
   - Verify DHCP is enabled
   - Try static IP configuration
   - Reboot router

5. **Debug via serial monitor**
   ```
   Connecting to WiFi: YourSSID
   .................
   Failed to connect to WiFi
   Starting Access Point mode...
   ```
   - Count dots (each = 500ms wait)
   - More than 20 dots = WiFi issue

### WiFi Connection Drops

**Symptoms:**
- Connects initially but loses connection
- Intermittent connectivity

**Solutions:**

1. **Improve signal strength**
   - Move closer to router
   - Add WiFi antenna (if supported)
   - Reduce interference sources

2. **Router configuration**
   - Disable AP isolation
   - Increase DHCP lease time
   - Use static IP for OmniLogger

3. **Power supply**
   - WiFi requires more current
   - Use quality power supply (1A minimum)
   - Add decoupling capacitor (100µF)

---

## SD Card Issues

### SD Card Not Detected

**Symptoms:**
- "SD card initialization failed" in serial monitor
- No data logging
- SD health shows error in web interface

**Solutions:**

1. **Check SD card**
   - Try different SD card
   - Format as FAT32 (not exFAT or NTFS)
   - Use SD card < 32GB for best compatibility
   - Avoid high-speed cards (UHS-II)

2. **Verify wiring**
   - Check all 6 connections (VCC, GND, CS, SCK, MOSI, MISO)
   - Ensure good contact (not loose)
   - Verify CS pin (default GPIO 12)

3. **Check power supply**
   - SD cards require stable 3.3V
   - Add 10µF capacitor near SD module
   - Ensure sufficient current (up to 200mA)

4. **Test with minimal setup**
   - Disconnect other sensors
   - Test SD card alone
   - Rule out power or interference issues

### SD Card Format Issues

**Symptoms:**
- Card detected but can't write files
- File system errors

**Solutions:**

1. **Format SD card properly:**
   
   **Windows:**
   ```
   1. Right-click SD card in File Explorer
   2. Select "Format..."
   3. File system: FAT32
   4. Allocation size: Default
   5. Quick Format: Checked
   6. Click Start
   ```
   
   **macOS:**
   ```bash
   diskutil list
   diskutil eraseDisk FAT32 OMNILOG /dev/diskX
   ```
   
   **Linux:**
   ```bash
   sudo mkfs.vfat -F 32 /dev/sdX1
   ```

2. **For cards > 32GB:**
   - Windows won't format > 32GB as FAT32
   - Use third-party tool (e.g., FAT32 Format)
   - Or use command line
   - Or partition to 32GB

### SD Card Fills Up

**Symptoms:**
- Storage full message
- Cannot write new data

**Solutions:**

1. **Download and clear old data**
   - Access web interface
   - Go to Data tab
   - Download important files
   - Delete old files (future feature)

2. **Use larger SD card**
   - Upgrade to larger card
   - Format as FAT32
   - Copy existing data if needed

3. **Increase measurement interval**
   - Reduce data frequency
   - Settings → Measurement Interval

---

## Sensor Problems

### BME280 Not Found

**Symptoms:**
- "Failed to initialize BME280" in serial monitor
- No temperature/humidity/pressure readings

**Solutions:**

1. **Check I2C connections**
   - Verify SDA → GPIO 33
   - Verify SCL → GPIO 35
   - Check VCC and GND

2. **Check I2C address**
   - Most BME280 use 0x76 or 0x77
   - Software tries both addresses
   - Verify module documentation

3. **Test I2C communication**
   - Use I2C scanner sketch (upload separate test)
   - Verify sensor responds

4. **Check for conflicts**
   - Disconnect other I2C devices
   - Test BME280 alone
   - Reconnect others one by one

### DHT22 Reading Errors

**Symptoms:**
- NaN (Not a Number) values
- Unreliable readings
- Timeout errors

**Solutions:**

1. **Check pull-up resistor**
   - Must have 4.7kΩ - 10kΩ resistor
   - Between DATA and VCC
   - Some modules include it

2. **Verify connections**
   - VCC, DATA, GND
   - Note: Pin 3 is not connected
   - Check DHT22 pinout diagram

3. **Check GPIO pin**
   - Verify pin number in configuration
   - Use recommended pins (GPIO 1-6, 11-18)
   - Avoid boot-sensitive pins

4. **Wire length**
   - Keep wires short (< 1 meter ideal)
   - Use shielded cable for long runs
   - Add 0.1µF capacitor near sensor

5. **Timing issues**
   - DHT22 requires 2 second intervals
   - Don't query too frequently
   - Software handles timing automatically

### DS18B20 Not Responding

**Symptoms:**
- Temperature shows -127°C or 85°C
- No readings
- Sensor not detected

**Solutions:**

1. **Check pull-up resistor (CRITICAL)**
   - **MUST** have 4.7kΩ resistor
   - Between DATA and VCC
   - Not optional for DS18B20

2. **Verify wiring**
   - Check pinout (GND, DATA, VCC)
   - TO-92 package: flat side facing you
   - Pin 1 (left) = GND
   - Pin 2 (middle) = DATA
   - Pin 3 (right) = VCC

3. **GPIO pin configuration**
   - Verify correct pin in web interface
   - Use any available GPIO
   - Avoid boot-sensitive pins

4. **Multiple sensors**
   - All share same 3 wires
   - Each has unique address
   - OneWire protocol handles addressing

5. **Cable length**
   - Works up to 100m with proper wiring
   - Use CAT5/6 cable for long runs
   - Twisted pair recommended

### Analog Sensor Issues

**Symptoms:**
- Erratic readings
- Values stuck at 0 or 4095
- Unexpected voltages

**Solutions:**

1. **Check voltage range**
   - ESP32 ADC: 0-3.3V only
   - **Never exceed 3.3V** on ADC pins
   - Use voltage divider for >3.3V

2. **Add capacitor**
   - 0.1µF between ADC pin and GND
   - Reduces noise
   - Stabilizes readings

3. **Calibration**
   - ADC may not be perfectly linear
   - Calibrate with known voltages
   - Apply correction in software

4. **Verify pin configuration**
   - Use ADC1 pins (GPIO 1-10)
   - Avoid WiFi interference (ADC2 unusable with WiFi)

5. **Sensor power**
   - Ensure sensor has stable power
   - Check sensor datasheet for requirements

---

## Web Interface Issues

### Cannot Access Web Interface

**Symptoms:**
- Browser cannot load page
- Connection timeout
- Page not found

**Solutions:**

1. **Verify IP address**
   - AP mode: http://192.168.4.1
   - STA mode: Check router or serial monitor
   - Try with and without http://

2. **Check WiFi connection**
   - Device connected to OmniLogger WiFi
   - Correct network (not mobile data)
   - WiFi enabled on device

3. **Try different browser**
   - Chrome/Edge recommended
   - Disable VPN
   - Try incognito/private mode
   - Clear browser cache

4. **Check firewall**
   - Disable firewall temporarily
   - Allow port 80
   - Check antivirus settings

### Web Interface Loads but Data Not Updating

**Symptoms:**
- Dashboard shows old data
- Stats not refreshing
- Readings stuck

**Solutions:**

1. **Check auto-refresh**
   - Dashboard auto-refreshes every 5 seconds
   - Manual refresh: switch tabs and back

2. **Verify sensors working**
   - Check serial monitor
   - Should see measurements being taken
   - Verify sensor configuration

3. **Browser cache**
   - Hard refresh (Ctrl+F5 or Cmd+Shift+R)
   - Clear browser cache
   - Try different browser

### Settings Won't Save

**Symptoms:**
- Save button pressed but settings revert
- "Error saving" message

**Solutions:**

1. **Check input validation**
   - Measurement interval > 0
   - Valid timezone offset (-12 to +14)
   - All required fields filled

2. **Flash memory**
   - NVS partition may be full
   - Erase and reflash firmware

3. **Reboot after saving**
   - Some settings require reboot
   - Use reboot button in web interface

---

## Data Logging Issues

### No Data Being Logged

**Symptoms:**
- Data point count stays at 0
- No CSV files created
- SD card empty

**Solutions:**

1. **Check SD card**
   - Verify SD card detected (see SD Card section)
   - Check SD health in dashboard

2. **Check measurement interval**
   - Verify interval > 0
   - Wait for next measurement cycle
   - Check serial monitor for "Taking Measurement"

3. **Check sensors**
   - At least one sensor must be enabled
   - Sensor must have valid readings
   - Verify sensor configuration

4. **Time synchronization**
   - NTP sync may be pending
   - Logs will use timestamp even if not synced
   - Check serial monitor

### CSV Files Empty or Corrupt

**Symptoms:**
- Files exist but no data
- Can't open CSV
- Garbled text

**Solutions:**

1. **Check file system**
   - Reformat SD card as FAT32
   - Test with different SD card

2. **Power during write**
   - Ensure stable power during logging
   - Don't remove SD card while running
   - Use flush() before deep sleep

3. **Download via web interface**
   - Don't remove SD card while powered
   - Download files through web UI
   - Verify download completes

### Timestamps Incorrect

**Symptoms:**
- Wrong date/time in logs
- Timezone issues

**Solutions:**

1. **Check NTP sync**
   - Verify WiFi connected
   - Check serial monitor: "Time synchronized"
   - May take 10-20 seconds after boot

2. **Configure timezone**
   - Settings → Timezone Offset
   - Hours from UTC (e.g., -5 for EST, +1 for CET)
   - Save and reboot

3. **No internet/NTP access**
   - Device uses epoch time if no NTP
   - Will show as large number
   - Can correct timestamps in post-processing

---

## Battery and Power Management

### Battery Voltage Shows 0.00V

**Symptoms:**
- Dashboard shows 0.00V battery
- Battery readings incorrect

**Solutions:**

1. **Check voltage divider**
   - Verify 10kΩ resistors installed
   - Check connections to GPIO 1
   - Verify resistor values with multimeter

2. **Check battery connection**
   - Battery connected and charged
   - Correct polarity
   - Voltage divider connected to battery+

3. **ADC pin**
   - Using GPIO 1 (ADC)
   - Not shorted to ground
   - Proper voltage divider ratio

### Deep Sleep Not Working

**Symptoms:**
- Device stays awake
- High power consumption
- Battery drains quickly

**Solutions:**

1. **Enable deep sleep**
   - Settings → Deep Sleep Mode → Enable
   - Save settings and reboot

2. **Check power source**
   - Deep sleep activates when on battery
   - USB power: stays awake (by design)
   - Check battery voltage < 5.0V

3. **Measurement interval**
   - Set appropriate wake interval
   - Too short = frequent wakes = high power

4. **USB connection**
   - USB keeps device awake
   - Disconnect USB for true deep sleep
   - Use battery power only

### Battery Life Shorter Than Expected

**Symptoms:**
- Battery drains too quickly
- Doesn't last as long as calculated

**Solutions:**

1. **Optimize settings**
   - Increase measurement interval
   - Enable deep sleep
   - Disable WiFi after configuration

2. **Check for power leaks**
   - Sensors drawing power in sleep
   - LED indicators
   - Voltage regulators with quiescent current

3. **Battery health**
   - Check battery voltage when fully charged
   - LiPo should be ~4.2V when full
   - Replace old/damaged batteries

4. **Calculation**
   - Measure actual current draw
   - Use multimeter in series
   - Calculate: battery mAh / avg current mA = hours

---

## Upload and Compilation Errors

### Upload Failed

**Symptoms:**
- "Failed to connect to ESP32"
- Upload timeout
- Permission errors

**Solutions:**

1. **Check USB connection**
   - Use data-capable USB cable
   - Try different USB port
   - Try different cable

2. **Put ESP32 in download mode**
   - Hold BOOT button
   - Press RESET button
   - Release RESET
   - Release BOOT
   - Try upload again

3. **Check serial port**
   - Verify correct COM port selected
   - Close serial monitor
   - Close other programs using port

4. **Driver issues (Windows)**
   - Install CH340/CP2102 drivers
   - Update drivers
   - Check Device Manager

### Compilation Errors

**Symptoms:**
- Build fails
- Library not found
- Syntax errors

**Solutions:**

1. **Install dependencies**
   ```bash
   pio lib install
   ```

2. **Update PlatformIO**
   ```bash
   pip install --upgrade platformio
   ```

3. **Clean build**
   ```bash
   pio run -t clean
   pio run
   ```

4. **Check library versions**
   - Verify platformio.ini
   - Update libraries if needed
   - Check for breaking changes

---

## Getting Help

If you've tried everything and still have issues:

1. **Check serial monitor output**
   - Provides detailed error messages
   - Shows boot sequence
   - Indicates where failure occurs

2. **Document your setup**
   - Hardware used
   - Wiring diagram
   - Configuration settings
   - Error messages

3. **Search for similar issues**
   - GitHub Issues
   - Arduino forums
   - ESP32 communities

4. **Open a GitHub Issue**
   - Include serial output
   - Describe steps to reproduce
   - List what you've already tried
   - Provide photos if relevant

## Useful Commands

### Serial Monitor
```bash
# PlatformIO
pio device monitor

# Or specify baud rate
pio device monitor -b 115200
```

### Erase Flash
```bash
pio run -t erase
```

### Upload Firmware
```bash
pio run -t upload
```

### View Build Output
```bash
pio run -v
```

---

## Preventive Maintenance

To avoid issues:

1. **Regular backups**
   - Download data files periodically
   - Keep backup of configuration

2. **Check SD card**
   - Replace every 1-2 years
   - Format occasionally
   - Monitor for errors

3. **Battery care**
   - Don't over-discharge (< 3.0V)
   - Don't overcharge (> 4.2V)
   - Use protection circuit

4. **Keep firmware updated**
   - Check for updates
   - Read changelog
   - Test before deploying

5. **Monitor system health**
   - Check dashboard regularly
   - Watch for anomalies
   - Address warnings promptly
