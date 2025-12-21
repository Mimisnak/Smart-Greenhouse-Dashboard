# ESP32-S3 Smart Greenhouse with Firebase

Production-ready PlatformIO project for ESP32-S3 smart greenhouse monitoring and control system.

## Features

- **Dynamic WiFi Configuration** via WiFiManager captive portal
- **Firebase Realtime Database** integration with legacy token authentication
- **Multi-sensor Support:**
  - BMP280: Temperature, Pressure, Altitude
  - BH1750: Light Intensity
  - Soil Moisture Sensor (Analog)
- **Water Pump Control** via relay (controlled from Firebase)
- **WiFi Reset Function** via BOOT button (5-second hold)

## Hardware Connections

| Component | Connection |
|-----------|------------|
| BMP280 | SDA=GPIO16, SCL=GPIO17, I2C Addr=0x76 |
| BH1750 | SDA=GPIO8, SCL=GPIO9, I2C Addr=0x23 |
| Soil Sensor | Analog GPIO4 |
| Relay | GPIO5 (Active HIGH) |
| BOOT Button | GPIO0 (WiFi Reset) |

## Soil Moisture Calibration

- **Dry Value:** 3285
- **Wet Value:** 27
- Output: 0-100% moisture level

## Firebase Setup

1. Open `src/main.cpp`
2. Replace these lines with your credentials:
   ```cpp
   #define FIREBASE_HOST "your-project-id.firebaseio.com"
   #define FIREBASE_AUTH "your-database-secret-token"
   ```

## Firebase Database Structure

```
greenhouse/
├── sensors/
│   ├── temperature (float)
│   ├── pressure (float)
│   ├── altitude (float)
│   ├── light (int)
│   ├── soil_moisture (int, 0-100%)
│   ├── soil_raw (int)
│   └── timestamp (int)
├── controls/
│   └── water_pump (int, 0=OFF, 1=ON)
├── status/
│   └── relay_state (int, 0=OFF, 1=ON)
└── system/
    ├── status (string)
    ├── device (string)
    └── startup_time (int)
```

## First Time Setup

1. **Install PlatformIO** in VS Code
2. **Open this folder** as a PlatformIO project
3. **Update Firebase credentials** in `src/main.cpp`
4. **Build and Upload:**
   ```
   pio run --target upload
   ```
5. **Monitor Serial Output:**
   ```
   pio device monitor
   ```

## WiFi Configuration

### Initial Setup:
1. Power on ESP32-S3
2. If no saved WiFi, it creates AP: **"Greenhouse_Connect"**
3. Connect to this AP with your phone/laptop
4. Captive portal opens automatically
5. Configure your WiFi credentials
6. ESP32 connects and starts operation

### Reset WiFi Settings:
1. Hold **BOOT button (GPIO0)** for **5 seconds**
2. ESP32 restarts and creates "Greenhouse_Connect" AP again

## Operation

- **Sensor Readings:** Every 30 seconds
- **Firebase Command Check:** Every 2 seconds
- **Water Pump Control:** Real-time from Firebase

## Libraries Used

- Firebase Arduino Client Library (ESP8266 & ESP32)
- WiFiManager by tzapu
- Adafruit BMP280 Library
- BH1750 by claws

## Serial Monitor Output

```
========================================
ESP32-S3 Smart Greenhouse System
========================================

[INIT] Initializing Sensors...
[OK] BMP280 initialized
[OK] BH1750 initialized
[OK] Soil Moisture Sensor initialized

[INIT] Starting WiFiManager...
[OK] WiFi Connected!
[INFO] IP Address: 192.168.1.100

[INIT] Connecting to Firebase...
[OK] Firebase connected successfully!

========================================
System Ready - Entering Main Loop
========================================
```

## Troubleshooting

**BMP280 Not Found:**
- Check I2C wiring (GPIO16/17)
- Verify I2C address (0x76 or 0x77)

**BH1750 Not Found:**
- Check I2C wiring (GPIO8/9)
- Verify I2C address (0x23)

**Firebase Connection Failed:**
- Verify FIREBASE_HOST and FIREBASE_AUTH
- Check internet connection
- Ensure database secret is valid

**WiFi Not Connecting:**
- Hold BOOT button for 5s to reset
- Reconnect to "Greenhouse_Connect" AP
- Re-enter WiFi credentials

## License

MIT License - Free for personal and commercial use
