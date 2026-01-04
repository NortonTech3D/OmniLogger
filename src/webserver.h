/*
 * Web Server for OmniLogger
 * Provides web interface for monitoring and configuration
 * 
 * Copyright (C) 2024 NortonTech3D
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"
#include "datalogger.h"

class WebServerManager {
public:
  WebServerManager() : server(80), config(nullptr), sensors(nullptr), logger(nullptr), 
                       getBatteryVoltage(nullptr), getWiFiEnabled(nullptr) {}
  
  void begin(Config* cfg, SensorManager* sens, DataLogger* log, 
             float (*batteryVoltageFn)() = nullptr, bool (*wifiEnabledFn)() = nullptr) {
    config = cfg;
    sensors = sens;
    logger = log;
    getBatteryVoltage = batteryVoltageFn;
    getWiFiEnabled = wifiEnabledFn;
    
    // Setup routes
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/sensors", HTTP_GET, [this]() { handleGetSensors(); });
    server.on("/api/sensors", HTTP_POST, [this]() { handleSetSensors(); });
    server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSetSettings(); });
    server.on("/api/data", HTTP_GET, [this]() { handleGetData(); });
    server.on("/api/files", HTTP_GET, [this]() { handleListFiles(); });
    server.on("/api/download", HTTP_GET, [this]() { handleDownload(); });
    server.on("/api/flush", HTTP_POST, [this]() { handleFlushBuffer(); });
    server.on("/style.css", HTTP_GET, [this]() { handleCSS(); });
    server.on("/script.js", HTTP_GET, [this]() { handleJS(); });
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    Serial.println("HTTP server started on port 80");
  }
  
  void handleClient() {
    server.handleClient();
  }

private:
  WebServer server;
  Config* config;
  SensorManager* sensors;
  DataLogger* logger;
  float (*getBatteryVoltage)();
  bool (*getWiFiEnabled)();
  
  void handleRoot() {
    String html = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OmniLogger Dashboard</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>🌡️ OmniLogger</h1>
            <p>Semi-Universal Data Logger</p>
        </header>
        
        <nav>
            <button onclick="showTab('dashboard')" class="tab-btn active" id="tab-dashboard">Dashboard</button>
            <button onclick="showTab('sensors')" class="tab-btn" id="tab-sensors">Sensors</button>
            <button onclick="showTab('settings')" class="tab-btn" id="tab-settings">Settings</button>
            <button onclick="showTab('data')" class="tab-btn" id="tab-data">Data</button>
        </nav>
        
        <div id="dashboard" class="tab-content active">
            <h2>System Status</h2>
            <div class="stats-grid">
                <div class="stat-card">
                    <h3>Data Points</h3>
                    <p class="stat-value" id="datapoints">-</p>
                </div>
                <div class="stat-card">
                    <h3>Battery Voltage</h3>
                    <p class="stat-value" id="battery">-</p>
                </div>
                <div class="stat-card">
                    <h3>Storage Used</h3>
                    <p class="stat-value" id="storage">-</p>
                </div>
                <div class="stat-card">
                    <h3>SD Card Health</h3>
                    <p class="stat-value" id="sdhealth">-</p>
                </div>
                <div class="stat-card">
                    <h3>Active Sensors</h3>
                    <p class="stat-value" id="sensorcount">-</p>
                </div>
                <div class="stat-card">
                    <h3>Uptime</h3>
                    <p class="stat-value" id="uptime">-</p>
                </div>
                <div class="stat-card">
                    <h3>Buffer Status</h3>
                    <p class="stat-value" id="buffer">-</p>
                </div>
                <div class="stat-card">
                    <h3>WiFi Status</h3>
                    <p class="stat-value" id="wifistatus">-</p>
                </div>
            </div>
            
            <div style="margin: 20px 0;">
                <button onclick="flushBuffer()" class="btn-primary">Flush Buffer to SD Card</button>
            </div>
            
            <h3>Current Readings</h3>
            <div id="readings" class="readings">
                <p>Loading...</p>
            </div>
        </div>
        
        <div id="sensors" class="tab-content">
            <h2>Sensor Configuration</h2>
            <p>Configure up to 8 sensors. Changes require a reboot to take effect.</p>
            <div id="sensor-config">
                <p>Loading...</p>
            </div>
            <button onclick="saveSensors()" class="btn-primary">Save Sensor Configuration</button>
        </div>
        
        <div id="settings" class="tab-content">
            <h2>System Settings</h2>
            <div class="settings-form">
                <h3>WiFi Station Configuration</h3>
                <label>WiFi SSID:</label>
                <input type="text" id="wifiSSID" placeholder="Network name">
                
                <label>WiFi Password:</label>
                <input type="password" id="wifiPassword" placeholder="Password">
                
                <h3>WiFi Access Point Configuration</h3>
                <label>AP SSID:</label>
                <input type="text" id="apSSID" placeholder="Access Point name">
                
                <label>AP Password:</label>
                <input type="password" id="apPassword" placeholder="AP Password (min 8 characters)" minlength="8">
                
                <h3>Data Buffering (Optional)</h3>
                <label>Enable Data Buffering:</label>
                <input type="checkbox" id="bufferingEnabled">
                <span>Store data in memory and flush periodically</span>
                
                <label>Flush Interval (seconds):</label>
                <input type="number" id="flushInterval" min="1" value="300">
                <span>How often to write buffered data to SD card</span>
                
                <h3>Measurement Settings</h3>
                <label>Measurement Interval (seconds):</label>
                <input type="number" id="measInterval" min="1" value="60">
                
                <label>Deep Sleep Mode:</label>
                <input type="checkbox" id="deepSleep">
                <span>Enable deep sleep between measurements (battery mode)</span>
                
                <h3>Time Settings</h3>
                <label>Timezone Offset (hours from UTC):</label>
                <input type="number" id="timezoneOffset" min="-12" max="14" value="0">
                
                <button onclick="saveSettings()" class="btn-primary">Save Settings</button>
                <button onclick="rebootDevice()" class="btn-warning">Reboot Device</button>
            </div>
        </div>
        
        <div id="data" class="tab-content">
            <h2>Data Files</h2>
            <button onclick="refreshFiles()" class="btn-secondary">Refresh</button>
            <div id="file-list">
                <p>Loading...</p>
            </div>
        </div>
    </div>
    
    <script src="/script.js"></script>
</body>
</html>)rawliteral";
    
    server.send(200, "text/html", html);
  }
  
  void handleCSS() {
    String css = R"rawliteral(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    padding: 20px;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    background: white;
    border-radius: 10px;
    box-shadow: 0 10px 40px rgba(0,0,0,0.2);
    overflow: hidden;
}

header {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 30px;
    text-align: center;
}

header h1 {
    font-size: 2.5em;
    margin-bottom: 10px;
}

nav {
    display: flex;
    background: #f5f5f5;
    border-bottom: 2px solid #ddd;
}

.tab-btn {
    flex: 1;
    padding: 15px;
    border: none;
    background: none;
    cursor: pointer;
    font-size: 16px;
    font-weight: 500;
    transition: all 0.3s;
}

.tab-btn:hover {
    background: #e0e0e0;
}

.tab-btn.active {
    background: white;
    border-bottom: 3px solid #667eea;
}

.tab-content {
    display: none;
    padding: 30px;
    animation: fadeIn 0.3s;
}

.tab-content.active {
    display: block;
}

@keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
}

.stats-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 20px;
    margin: 20px 0;
}

.stat-card {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 20px;
    border-radius: 10px;
    text-align: center;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
}

.stat-card h3 {
    font-size: 14px;
    margin-bottom: 10px;
    opacity: 0.9;
}

.stat-value {
    font-size: 24px;
    font-weight: bold;
}

.readings {
    background: #f9f9f9;
    padding: 20px;
    border-radius: 8px;
    margin-top: 20px;
}

.sensor-item {
    background: white;
    padding: 15px;
    margin: 10px 0;
    border-radius: 8px;
    border-left: 4px solid #667eea;
}

.settings-form label {
    display: block;
    margin-top: 15px;
    margin-bottom: 5px;
    font-weight: 500;
}

.settings-form input[type="text"],
.settings-form input[type="password"],
.settings-form input[type="number"],
.settings-form select {
    width: 100%;
    padding: 10px;
    border: 1px solid #ddd;
    border-radius: 5px;
    font-size: 14px;
}

.settings-form h3 {
    margin-top: 25px;
    margin-bottom: 15px;
    color: #667eea;
}

.btn-primary, .btn-secondary, .btn-warning {
    padding: 12px 24px;
    border: none;
    border-radius: 5px;
    font-size: 16px;
    cursor: pointer;
    margin: 10px 5px 0 0;
    transition: all 0.3s;
}

.btn-primary {
    background: #667eea;
    color: white;
}

.btn-primary:hover {
    background: #5568d3;
}

.btn-secondary {
    background: #6c757d;
    color: white;
}

.btn-secondary:hover {
    background: #5a6268;
}

.btn-warning {
    background: #ffc107;
    color: #000;
}

.btn-warning:hover {
    background: #e0a800;
}

#file-list {
    background: #f9f9f9;
    padding: 20px;
    border-radius: 8px;
    margin-top: 20px;
}

.file-item {
    background: white;
    padding: 12px;
    margin: 8px 0;
    border-radius: 5px;
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.file-item button {
    padding: 6px 12px;
    background: #667eea;
    color: white;
    border: none;
    border-radius: 4px;
    cursor: pointer;
}

.file-item button:hover {
    background: #5568d3;
}
)rawliteral";
    
    server.send(200, "text/css", css);
  }
  
  void handleJS() {
    String js = R"rawliteral(
let statusInterval;

function showTab(tabName) {
    // Hide all tabs
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
    });
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    
    // Show selected tab
    document.getElementById(tabName).classList.add('active');
    document.getElementById('tab-' + tabName).classList.add('active');
    
    // Load tab-specific data
    if (tabName === 'dashboard') {
        loadStatus();
        if (!statusInterval) {
            statusInterval = setInterval(loadStatus, 5000);
        }
    } else {
        if (statusInterval) {
            clearInterval(statusInterval);
            statusInterval = null;
        }
        
        if (tabName === 'sensors') {
            loadSensors();
        } else if (tabName === 'settings') {
            loadSettings();
        } else if (tabName === 'data') {
            refreshFiles();
        }
    }
}

function loadStatus() {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('datapoints').textContent = data.datapoints.toLocaleString();
            document.getElementById('battery').textContent = data.battery.toFixed(2) + 'V';
            document.getElementById('storage').textContent = data.storageUsed + ' / ' + data.storageTotal;
            document.getElementById('sdhealth').textContent = data.sdHealthy ? '✓ Healthy' : '✗ Error';
            document.getElementById('sensorcount').textContent = data.sensorCount;
            document.getElementById('uptime').textContent = formatUptime(data.uptime);
            document.getElementById('buffer').textContent = data.bufferCount + ' / ' + data.bufferCapacity;
            document.getElementById('wifistatus').textContent = data.wifiEnabled ? '✓ Enabled' : '✗ Disabled';
            
            // Update readings
            let readingsHTML = '';
            data.readings.forEach(reading => {
                readingsHTML += '<div class="sensor-item">';
                readingsHTML += '<h4>' + reading.name + '</h4>';
                readingsHTML += '<p>' + reading.data + '</p>';
                readingsHTML += '</div>';
            });
            document.getElementById('readings').innerHTML = readingsHTML || '<p>No sensor readings available</p>';
        })
        .catch(err => console.error('Error loading status:', err));
}

function loadSensors() {
    fetch('/api/sensors')
        .then(response => response.json())
        .then(data => {
            let html = '';
            data.sensors.forEach((sensor, index) => {
                html += '<div class="sensor-item">';
                html += '<h4>Sensor ' + (index + 1) + '</h4>';
                html += '<label>Enabled:</label>';
                html += '<input type="checkbox" id="s' + index + '_enabled" ' + (sensor.enabled ? 'checked' : '') + '><br>';
                html += '<label>Name:</label>';
                html += '<input type="text" id="s' + index + '_name" value="' + sensor.name + '"><br>';
                html += '<label>Type:</label>';
                html += '<select id="s' + index + '_type">';
                html += '<option value="0"' + (sensor.type === 0 ? ' selected' : '') + '>None</option>';
                html += '<option value="1"' + (sensor.type === 1 ? ' selected' : '') + '>BME280 (I2C)</option>';
                html += '<option value="2"' + (sensor.type === 2 ? ' selected' : '') + '>DHT22</option>';
                html += '<option value="3"' + (sensor.type === 3 ? ' selected' : '') + '>DS18B20</option>';
                html += '<option value="4"' + (sensor.type === 4 ? ' selected' : '') + '>Analog</option>';
                html += '</select><br>';
                html += '<label>Pin (for digital/analog sensors):</label>';
                html += '<input type="number" id="s' + index + '_pin" value="' + sensor.pin + '">';
                html += '</div>';
            });
            document.getElementById('sensor-config').innerHTML = html;
        })
        .catch(err => console.error('Error loading sensors:', err));
}

function saveSensors() {
    let sensors = [];
    for (let i = 0; i < 8; i++) {
        let enabled = document.getElementById('s' + i + '_enabled');
        if (enabled) {
            sensors.push({
                enabled: enabled.checked,
                name: document.getElementById('s' + i + '_name').value,
                type: parseInt(document.getElementById('s' + i + '_type').value),
                pin: parseInt(document.getElementById('s' + i + '_pin').value)
            });
        }
    }
    
    fetch('/api/sensors', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({sensors: sensors})
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message);
    })
    .catch(err => {
        alert('Error saving sensors: ' + err);
    });
}

function loadSettings() {
    fetch('/api/settings')
        .then(response => response.json())
        .then(data => {
            document.getElementById('wifiSSID').value = data.wifiSSID || '';
            document.getElementById('wifiPassword').value = '';
            document.getElementById('apSSID').value = data.apSSID || '';
            document.getElementById('apPassword').value = '';
            document.getElementById('bufferingEnabled').checked = data.bufferingEnabled || false;
            document.getElementById('flushInterval').value = data.flushInterval || 300;
            document.getElementById('measInterval').value = data.measurementInterval;
            document.getElementById('deepSleep').checked = data.deepSleepEnabled;
            document.getElementById('timezoneOffset').value = data.timezoneOffset;
        })
        .catch(err => console.error('Error loading settings:', err));
}

function saveSettings() {
    const settings = {
        wifiSSID: document.getElementById('wifiSSID').value,
        wifiPassword: document.getElementById('wifiPassword').value,
        apSSID: document.getElementById('apSSID').value,
        apPassword: document.getElementById('apPassword').value,
        bufferingEnabled: document.getElementById('bufferingEnabled').checked,
        flushInterval: parseInt(document.getElementById('flushInterval').value),
        measurementInterval: parseInt(document.getElementById('measInterval').value),
        deepSleepEnabled: document.getElementById('deepSleep').checked,
        timezoneOffset: parseInt(document.getElementById('timezoneOffset').value)
    };
    
    fetch('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(settings)
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message);
    })
    .catch(err => {
        alert('Error saving settings: ' + err);
    });
}

function rebootDevice() {
    if (confirm('Are you sure you want to reboot the device?')) {
        fetch('/api/settings', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({reboot: true})
        })
        .then(() => {
            alert('Device is rebooting...');
        })
        .catch(err => {
            alert('Error rebooting device: ' + err);
        });
    }
}

function refreshFiles() {
    fetch('/api/files')
        .then(response => response.json())
        .then(data => {
            let html = '';
            data.files.forEach(file => {
                html += '<div class="file-item">';
                html += '<span>' + file.name + ' (' + file.size + ' bytes)</span>';
                html += '<button onclick="downloadFile(\'' + file.name + '\')">Download</button>';
                html += '</div>';
            });
            document.getElementById('file-list').innerHTML = html || '<p>No data files found</p>';
        })
        .catch(err => console.error('Error loading files:', err));
}

function downloadFile(filename) {
    window.open('/api/download?file=' + encodeURIComponent(filename), '_blank');
}

function flushBuffer() {
    if (!confirm('Flush buffered data to SD card now?')) {
        return;
    }
    
    fetch('/api/flush', {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || 'Buffer flushed successfully');
        loadStatus();  // Refresh status to show updated buffer count
    })
    .catch(err => {
        alert('Error flushing buffer: ' + err);
    });
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const mins = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) {
        return days + 'd ' + hours + 'h ' + mins + 'm';
    } else if (hours > 0) {
        return hours + 'h ' + mins + 'm';
    } else {
        return mins + 'm';
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    showTab('dashboard');
});
)rawliteral";
    
    server.send(200, "application/javascript", js);
  }
  
  void handleStatus() {
    DynamicJsonDocument doc(2048);
    
    // System stats
    doc["datapoints"] = logger->getDataPointCount();
    doc["battery"] = getBatteryVoltage ? getBatteryVoltage() : 0.0f;
    doc["storageTotal"] = String(logger->getTotalSize() / (1024*1024)) + "MB";
    doc["storageUsed"] = String(logger->getUsedSize() / (1024*1024)) + "MB";
    doc["sdHealthy"] = logger->isHealthy();
    doc["sensorCount"] = sensors->getSensorCount();
    doc["uptime"] = millis() / 1000;  // Note: Resets after ~49.7 days due to millis() overflow
    
    // Buffer stats
    doc["bufferCount"] = logger->getBufferCount();
    doc["bufferCapacity"] = logger->getBufferCapacity();
    
    // WiFi status
    doc["wifiEnabled"] = getWiFiEnabled ? getWiFiEnabled() : true;
    
    // Current sensor readings
    JsonArray readings = doc.createNestedArray("readings");
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      SensorType type = sensors->getSensorType(i);
      if (type == SENSOR_NONE) continue;
      
      SensorReading reading = sensors->getReading(i);
      if (!reading.valid) continue;
      
      JsonObject r = readings.createNestedObject();
      r["name"] = sensors->getSensorName(i);
      
      String data = "";
      switch (type) {
        case SENSOR_BME280:
          data = "Temp: " + String(reading.temperature, 1) + "°C, " +
                 "Humidity: " + String(reading.humidity, 1) + "%, " +
                 "Pressure: " + String(reading.pressure, 1) + "hPa";
          break;
        case SENSOR_DHT22:
          data = "Temp: " + String(reading.temperature, 1) + "°C, " +
                 "Humidity: " + String(reading.humidity, 1) + "%";
          break;
        case SENSOR_DS18B20:
          data = "Temp: " + String(reading.temperature, 1) + "°C";
          break;
        case SENSOR_ANALOG:
          data = "Value: " + String(reading.value, 2);
          break;
        default:
          break;
      }
      r["data"] = data;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  
  void handleGetSensors() {
    DynamicJsonDocument doc(2048);
    JsonArray sensorsArray = doc.createNestedArray("sensors");
    
    for (int i = 0; i < Config::MAX_SENSORS; i++) {
      JsonObject s = sensorsArray.createNestedObject();
      s["enabled"] = config->sensors[i].enabled;
      s["name"] = config->sensors[i].name;
      s["type"] = (int)config->sensors[i].type;
      s["pin"] = config->sensors[i].pin;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  
  void handleSetSensors() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, server.arg("plain"));
      
      if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      JsonArray sensorsArray = doc["sensors"];
      if (!sensorsArray.isNull()) {
        for (int i = 0; i < Config::MAX_SENSORS && i < (int)sensorsArray.size(); i++) {
          JsonObject s = sensorsArray[i];
          
          // Validate and update enabled status
          if (s.containsKey("enabled")) {
            config->sensors[i].enabled = s["enabled"];
          }
          
          // Validate and update sensor name
          if (s.containsKey("name")) {
            const char* name = s["name"];
            if (strlen(name) < 32) {
              strncpy(config->sensors[i].name, name, sizeof(config->sensors[i].name) - 1);
              config->sensors[i].name[sizeof(config->sensors[i].name) - 1] = '\0';
            }
          }
          
          // Validate and update sensor type
          if (s.containsKey("type")) {
            int type = s["type"];
            if (type >= SENSOR_NONE && type <= SENSOR_ANALOG) {
              config->sensors[i].type = (SensorType)type;
            }
          }
          
          // Validate and update pin number
          if (s.containsKey("pin")) {
            int pin = s["pin"];
            if (config->validatePinNumber(pin) || pin == -1 || pin == 0) {
              config->sensors[i].pin = pin;
            }
          }
        }
      }
      
      config->save();
      
      DynamicJsonDocument response(256);
      response["success"] = true;
      response["message"] = "Sensor configuration saved! Please reboot for changes to take effect.";
      
      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
  }
  
  void handleGetSettings() {
    DynamicJsonDocument doc(512);
    
    doc["wifiSSID"] = config->wifiSSID;
    doc["apSSID"] = config->apSSID;
    // Don't send passwords
    doc["bufferingEnabled"] = config->bufferingEnabled;
    doc["flushInterval"] = config->flushInterval;
    doc["measurementInterval"] = config->measurementInterval;
    doc["deepSleepEnabled"] = config->deepSleepEnabled;
    doc["timezoneOffset"] = config->timezoneOffset;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  
  void handleSetSettings() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, server.arg("plain"));
      
      if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      if (doc.containsKey("reboot") && doc["reboot"]) {
        server.send(200, "application/json", "{\"message\":\"Rebooting...\"}");
        delay(1000);
        ESP.restart();
        return;
      }
      
      // Validate and update WiFi SSID
      if (doc.containsKey("wifiSSID")) {
        const char* ssid = doc["wifiSSID"];
        if (strlen(ssid) < 64) {
          strncpy(config->wifiSSID, ssid, sizeof(config->wifiSSID) - 1);
          config->wifiSSID[sizeof(config->wifiSSID) - 1] = '\0';
        }
      }
      
      // Validate and update WiFi password
      if (doc.containsKey("wifiPassword") && strlen(doc["wifiPassword"]) > 0) {
        const char* pass = doc["wifiPassword"];
        if (strlen(pass) < 64) {
          strncpy(config->wifiPassword, pass, sizeof(config->wifiPassword) - 1);
          config->wifiPassword[sizeof(config->wifiPassword) - 1] = '\0';
        }
      }
      
      // Validate and update AP SSID
      if (doc.containsKey("apSSID")) {
        const char* ssid = doc["apSSID"];
        if (strlen(ssid) > 0 && strlen(ssid) < 64) {
          strncpy(config->apSSID, ssid, sizeof(config->apSSID) - 1);
          config->apSSID[sizeof(config->apSSID) - 1] = '\0';
        }
      }
      
      // Validate and update AP password (must be 8+ characters)
      if (doc.containsKey("apPassword") && strlen(doc["apPassword"]) > 0) {
        const char* apPass = doc["apPassword"];
        if (config->validateAPPassword(apPass)) {
          strncpy(config->apPassword, apPass, sizeof(config->apPassword) - 1);
          config->apPassword[sizeof(config->apPassword) - 1] = '\0';
        }
      }
      
      // Update buffering settings
      if (doc.containsKey("bufferingEnabled")) {
        config->bufferingEnabled = doc["bufferingEnabled"];
      }
      
      // Validate and update flush interval
      if (doc.containsKey("flushInterval")) {
        unsigned int interval = doc["flushInterval"];
        if (config->validateFlushInterval(interval)) {
          config->flushInterval = interval;
        }
      }
      
      // Validate and update measurement interval
      if (doc.containsKey("measurementInterval")) {
        unsigned int interval = doc["measurementInterval"];
        if (config->validateMeasurementInterval(interval)) {
          config->measurementInterval = interval;
        }
      }
      
      // Update deep sleep setting
      if (doc.containsKey("deepSleepEnabled")) {
        config->deepSleepEnabled = doc["deepSleepEnabled"];
      }
      
      // Validate and update timezone offset
      if (doc.containsKey("timezoneOffset")) {
        int offset = doc["timezoneOffset"];
        if (config->validateTimezoneOffset(offset)) {
          config->timezoneOffset = offset;
        }
      }
      
      config->save();
      
      DynamicJsonDocument response(256);
      response["success"] = true;
      response["message"] = "Settings saved successfully!";
      
      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
  }
  
  void handleGetData() {
    // Get query parameters
    String filename = server.arg("file");
    int limit = server.hasArg("limit") ? server.arg("limit").toInt() : 100;
    
    // Validate limit parameter
    if (limit < 1 || limit > 1000) {
      limit = 100;  // Default to safe value
    }
    
    if (filename.length() == 0) {
      // If no file specified, return error
      server.send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
      return;
    }
    
    // Security: Prevent directory traversal attacks
    if (filename.indexOf("..") != -1 || filename.indexOf("\\") != -1) {
      server.send(400, "application/json", "{\"error\":\"Invalid file path\"}");
      return;
    }
    
    // Ensure filename starts with /
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    
    // Stream file directly instead of loading into memory
    // For this version, we'll use a safer approach with streaming
    // Note: For large files, consider pagination or streaming API
    
    // Calculate JSON document size based on limit
    // Each line might have ~200 bytes in JSON, plus overhead
    size_t docSize = std::min((size_t)16384, (size_t)(limit * 250 + 512));
    DynamicJsonDocument doc(docSize);
    JsonArray dataArray = doc.createNestedArray("data");
    
    // Open file for reading
    String content;
    if (!logger->downloadFile(filename.c_str(), content)) {
      server.send(404, "application/json", "{\"error\":\"File not found\"}");
      return;
    }
    
    // Check if content is too large
    if (content.length() > 50000) {  // ~50KB limit for safety
      server.send(413, "application/json", "{\"error\":\"File too large, use download instead\"}");
      return;
    }
    
    // Split content into lines
    int lineCount = 0;
    int startPos = 0;
    int endPos = content.indexOf('\n');
    String header = "";
    
    while (endPos != -1 && lineCount < limit) {
      String line = content.substring(startPos, endPos);
      line.trim();
      
      if (line.length() > 0) {
        if (lineCount == 0) {
          // First line is header
          header = line;
        } else {
          // Data lines
          JsonObject row = dataArray.createNestedObject();
          
          // Parse CSV values
          int colIndex = 0;
          int valueStart = 0;
          int valueEnd = line.indexOf(',');
          int headerStart = 0;
          int headerEnd = header.indexOf(',');
          
          // Improved loop condition to prevent infinite loops
          while (valueStart < (int)line.length() && colIndex < 50) {  // Max 50 columns for safety
            // Get column name from header
            String colName;
            if (headerEnd != -1) {
              colName = header.substring(headerStart, headerEnd);
              headerStart = headerEnd + 1;
              headerEnd = header.indexOf(',', headerStart);
            } else if (headerStart < (int)header.length()) {
              colName = header.substring(headerStart);
            } else {
              break;  // No more header columns
            }
            colName.trim();
            
            // Get value
            String value;
            if (valueEnd != -1) {
              value = line.substring(valueStart, valueEnd);
              valueStart = valueEnd + 1;
              valueEnd = line.indexOf(',', valueStart);
            } else {
              value = line.substring(valueStart);
              valueStart = line.length();  // This will end the loop
            }
            value.trim();
            
            // Add to JSON object
            if (colName.length() > 0) {
              row[colName] = value;
            }
            
            colIndex++;
          }
        }
        lineCount++;
      }
      
      startPos = endPos + 1;
      endPos = content.indexOf('\n', startPos);
    }
    
    doc["count"] = dataArray.size();
    doc["file"] = filename;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  
  void handleListFiles() {
    String fileList;
    logger->listFiles(fileList);
    
    DynamicJsonDocument doc(2048);
    JsonArray filesArray = doc.createNestedArray("files");
    
    // Parse file list
    int startPos = 0;
    int endPos = fileList.indexOf('\n');
    while (endPos != -1) {
      String line = fileList.substring(startPos, endPos);
      int sizePos = line.lastIndexOf('(');
      if (sizePos != -1) {
        String filename = line.substring(0, sizePos - 1);
        String sizeStr = line.substring(sizePos + 1, line.indexOf(' ', sizePos));
        
        JsonObject file = filesArray.createNestedObject();
        file["name"] = filename;
        file["size"] = sizeStr;
      }
      
      startPos = endPos + 1;
      endPos = fileList.indexOf('\n', startPos);
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  
  void handleDownload() {
    if (server.hasArg("file")) {
      String filename = server.arg("file");
      
      // Security: Prevent directory traversal attacks
      if (filename.indexOf("..") != -1 || filename.indexOf("\\") != -1) {
        server.send(400, "text/plain", "Invalid file path");
        return;
      }
      
      // Ensure filename starts with /
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      
      if (logger->streamFile(filename.c_str(), server)) {
        // File streamed successfully
      } else {
        server.send(404, "text/plain", "File not found");
      }
    } else {
      server.send(400, "text/plain", "Missing file parameter");
    }
  }
  
  void handleFlushBuffer() {
    DynamicJsonDocument doc(256);
    
    int bufferCount = logger->getBufferCount();
    
    if (bufferCount == 0) {
      doc["message"] = "Buffer is empty - nothing to flush";
      doc["success"] = true;
    } else {
      bool success = logger->flushBuffer();
      if (success) {
        doc["message"] = String("Successfully flushed ") + String(bufferCount) + " data points to SD card";
        doc["success"] = true;
      } else {
        doc["message"] = "Failed to flush buffer - check SD card";
        doc["success"] = false;
      }
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  
  void handleNotFound() {
    server.send(404, "text/plain", "404: Not found");
  }
};

#endif // WEBSERVER_H
