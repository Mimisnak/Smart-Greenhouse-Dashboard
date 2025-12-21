 * ESP32-S3 Smart Greenhouse with Firebase & WiFiManager
 * ...existing code...
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <Adafruit_NeoPixel.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ...existing code...
// ========== MAIN LOOP ========== 

/**
 * ESP32-S3 Smart Greenhouse with Firebase & WiFiManager
 * 
 * Features:
 * - Dynamic WiFi configuration via captive portal
 * - Firebase Realtime Database integration
 * - BMP280 (Temperature, Pressure, Humidity)
 * - BH1750 (Light Intensity)
 * - Soil Moisture Sensor
 * - Water Pump Relay Control
 * - WiFi Reset via BOOT button (5s hold)
 * 
 * Hardware Connections:
 * - BMP280: SDA=16, SCL=17, I2C Addr=0x76
 * - BH1750: SDA=8, SCL=9, I2C Addr=0x23
 * - Soil Sensor: Analog GPIO 4
 * - Relay: GPIO 5 (Active HIGH)
 * - BOOT Button: GPIO 0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <Adafruit_NeoPixel.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ========== PIN DEFINITIONS ========== ...existing code...
// ========== MAIN LOOP ==========
