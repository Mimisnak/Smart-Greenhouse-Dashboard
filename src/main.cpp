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

// ========== PIN DEFINITIONS ==========
#define BMP280_SDA 16
#define BMP280_SCL 17
#define BH1750_SDA 8
#define BH1750_SCL 9
#define SOIL_SENSOR_PIN 4
#define RELAY_PIN 5
#define BOOT_BUTTON_PIN 0
#define RGB_LED_PIN 48  // WS2812 RGB LED on ESP32-S3 DevKitC-1

// ========== SENSOR CALIBRATION ==========
#define SOIL_DRY_VALUE 3285  // Ξηρό (στον αέρα/ξύλο) - Βασισμένο στις μετρήσεις
#define SOIL_WET_VALUE 1300  // Βρεγμένο χώμα (στο νερό)

// ========== TIMING CONSTANTS ==========
#define SENSOR_READ_INTERVAL 5000   // 5 seconds
#define FIREBASE_CHECK_INTERVAL 2000 // 2 seconds
#define BUTTON_HOLD_TIME 5000        // 5 seconds for WiFi reset
#define DEBOUNCE_DELAY 50            // 50ms debounce

// ========== FIREBASE CONFIG ==========
// REPLACE WITH YOUR FIREBASE CREDENTIALS
#define FIREBASE_HOST "smartgreenhouse-fb494-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "4JH5iFyDrTQxA6qR1x4iKiQNrCaAH3ZdrqMEpHOT"

// ========== GLOBAL OBJECTS ==========
Adafruit_BMP280 bmp;
BH1750 lightMeter;
WiFiManager wm;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ========== STATE VARIABLES ==========
unsigned long lastSensorRead = 0;
unsigned long lastFirebaseCheck = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool firebaseReady = false;
bool relayState = false;

// Auto-watering variables
bool autoWateringEnabled = false;
int moistureThreshold = 30;  // Default 30%
unsigned long pumpStartTime = 0;
unsigned long pumpDuration = 0;
bool pumpTimerActive = false;
unsigned long lastAutoWatering = 0;
#define AUTO_WATERING_COOLDOWN 5000  // 5 seconds cooldown for TESTING (change back to 300000 for production!)

// History upload
unsigned long lastHistoryUpload = 0;
#define HISTORY_UPLOAD_INTERVAL 30000  // Upload to Firebase history every 30 seconds

// ========== SENSOR DATA STRUCTURE ==========
struct SensorData {
    float temperature;
    float pressure;
    float altitude;
    uint16_t lightLevel;
    int soilMoisture;
    int soilRaw;
};

SensorData sensors;

// ========== RGB LED ==========
Adafruit_NeoPixel rgbLED(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// ========== FUNCTION PROTOTYPES ==========
void initializeSerial();
void initializeSensors();
void initializeRelay();
void initializeWiFi();
void initializeFirebase();
void readSensors();
void uploadSensorsToFirebase();
void checkFirebaseCommands();
void controlRelay(bool state);
void checkButtonForReset();
void startPumpTimer(unsigned long duration);
void stopPumpTimer();
int mapSoilMoisture(int raw);
void flashRGBLED(uint8_t r, uint8_t g, uint8_t b, int duration);

// ========== SETUP ==========
void setup() {
    // CRITICAL: Set relay OFF immediately to prevent unwanted activation during boot
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    
    initializeSerial();
    Serial.println("\n\n========================================");
    Serial.println("ESP32-S3 Smart Greenhouse System");
    Serial.println("========================================\n");
    
    // Initialize RGB LED
    rgbLED.begin();
    rgbLED.setBrightness(50);  // 50/255 brightness
    rgbLED.setPixelColor(0, rgbLED.Color(0, 0, 255));  // Blue = starting up
    rgbLED.show();
    
    initializeRelay();
    initializeSensors();
    initializeWiFi();
    initializeFirebase();
    
    Serial.println("\n========================================");
    Serial.println("System Ready - Entering Main Loop");
    Serial.println("========================================\n");
}

// ========== MAIN LOOP ==========
void loop() {
    unsigned long currentMillis = millis();
    
    // Check pump timer FIRST (highest priority)
    if (pumpTimerActive) {
        unsigned long elapsed = currentMillis - pumpStartTime;
        if (elapsed >= pumpDuration) {
            stopPumpTimer();
        } else {
            // Update remaining time in Firebase every second
            static unsigned long lastTimerUpdate = 0;
            if (currentMillis - lastTimerUpdate >= 1000) {
                lastTimerUpdate = currentMillis;
                unsigned long remaining = (pumpDuration - elapsed) / 1000;
                if (firebaseReady && Firebase.ready()) {
                    Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/pump_timer_remaining", remaining);
                }
                Serial.printf("[TIMER] Pump running: %lu seconds remaining\n", remaining);
            }
        }
    }
    
    // Read and upload sensors every 5 seconds
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;
        readSensors();
        uploadSensorsToFirebase();
        
        // Auto-watering logic with cooldown
        if (autoWateringEnabled) {
            if (sensors.soilMoisture < moistureThreshold) {
                if (!pumpTimerActive) {
                    // Check if cooldown period has passed (5 minutes)
                    if (currentMillis - lastAutoWatering >= AUTO_WATERING_COOLDOWN || lastAutoWatering == 0) {
                        Serial.printf("[AUTO] Soil moisture low (%d%% < %d%%), starting pump for 15 seconds\n", sensors.soilMoisture, moistureThreshold);
                        startPumpTimer(15000); // 15 seconds
                        lastAutoWatering = currentMillis;
                    } else {
                        unsigned long remainingCooldown = (AUTO_WATERING_COOLDOWN - (currentMillis - lastAutoWatering)) / 1000;
                        if (remainingCooldown % 30 == 0) { // Print every 30 seconds
                            Serial.printf("[AUTO] Cooldown active, next watering in %lu seconds\n", remainingCooldown);
                        }
                    }
                } else {
                    Serial.println("[AUTO] Pump timer already active, skipping");
                }
            } else {
                static unsigned long lastOkPrint = 0;
                if (currentMillis - lastOkPrint >= 30000) { // Print every 30 seconds
                    Serial.printf("[AUTO] Moisture OK (%d%% >= %d%%)\n", sensors.soilMoisture, moistureThreshold);
                    lastOkPrint = currentMillis;
                }
            }
        }
    }
    
    // Check Firebase commands every 2 seconds
    if (currentMillis - lastFirebaseCheck >= FIREBASE_CHECK_INTERVAL) {
        lastFirebaseCheck = currentMillis;
        checkFirebaseCommands();
    }
    
    // Check for WiFi reset button
    checkButtonForReset();
    
    // Small delay for task stability
    vTaskDelay(pdMS_TO_TICKS(10));
}

// ========== INITIALIZATION FUNCTIONS ==========

void initializeSerial() {
    Serial.begin(115200);
    delay(1000);
}

void initializeSensors() {
    Serial.println("[INIT] Initializing Sensors...");
    
    // Initialize BMP280 I2C (Wire on custom pins)
    Wire.begin(BMP280_SDA, BMP280_SCL);
    if (!bmp.begin(0x76)) {
        Serial.println("[ERROR] BMP280 not found! Check wiring.");
        Serial.println("[INFO] System will continue without BMP280.");
    } else {
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                       Adafruit_BMP280::SAMPLING_X2,
                       Adafruit_BMP280::SAMPLING_X16,
                       Adafruit_BMP280::FILTER_X16,
                       Adafruit_BMP280::STANDBY_MS_500);
        Serial.println("[OK] BMP280 initialized");
    }
    
    // Initialize BH1750 I2C (Wire1 on custom pins)
    Wire1.begin(BH1750_SDA, BH1750_SCL);
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1)) {
        Serial.println("[OK] BH1750 initialized");
    } else {
        Serial.println("[ERROR] BH1750 not found! Check wiring.");
        Serial.println("[INFO] System will continue without BH1750.");
    }
    
    // Initialize Soil Sensor (Analog)
    pinMode(SOIL_SENSOR_PIN, INPUT);
    Serial.println("[OK] Soil Moisture Sensor initialized");
    
    Serial.println("[OK] All sensors initialized\n");
}

void initializeRelay() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
    Serial.println("[OK] Relay initialized (OFF)\n");
}

void initializeWiFi() {
    Serial.println("[INIT] Starting WiFiManager...");
    Serial.println("[INFO] If not connected, AP 'Greenhouse_Connect' will start");
    Serial.println("[INFO] Hold BOOT button for 5s to reset WiFi settings\n");
    
    // Configure button for WiFi reset
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    
    // WiFiManager configuration
    wm.setConfigPortalTimeout(180); // 3 minutes timeout
    wm.setConnectTimeout(30);       // 30 seconds to connect
    
    // Blocking call - will start AP if needed
    if (!wm.autoConnect("Greenhouse_Connect")) {
        Serial.println("[ERROR] Failed to connect and timeout occurred");
        Serial.println("[INFO] Restarting ESP32...");
        delay(3000);
        ESP.restart();
    }
    
    // WiFi Connected
    Serial.println("\n[OK] WiFi Connected!");
    Serial.print("[INFO] IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[INFO] Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
    
    // Set RGB LED to off after WiFi connected (will blink blue on activity)
    rgbLED.setPixelColor(0, rgbLED.Color(0, 0, 0));  // Off
    rgbLED.show();
}

void initializeFirebase() {
    Serial.println("[INIT] Connecting to Firebase...");
    
    // Configure Firebase
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    
    // Set timeouts
    config.timeout.serverResponse = 10 * 1000;
    
    // Initialize Firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    // Set buffer sizes for larger payloads
    fbdo.setBSSLBufferSize(1024, 1024);
    
    // Test connection
    Serial.println("[INFO] Testing Firebase connection...");
    if (Firebase.ready()) {
        firebaseReady = true;
        Serial.println("[OK] Firebase connected successfully!\n");
        
        // Set initial system status
        Firebase.RTDB.setString(&fbdo, "/greenhouse/system/status", "online");
        Firebase.RTDB.setString(&fbdo, "/greenhouse/system/device", "ESP32-S3");
        Firebase.RTDB.setInt(&fbdo, "/greenhouse/system/startup_time", millis());
        
        // Read or set default moisture threshold
        if (Firebase.RTDB.getInt(&fbdo, "/greenhouse/controls/moisture_threshold")) {
            if (fbdo.dataType() == "int") {
                int savedThreshold = fbdo.intData();
                if (savedThreshold > 0 && savedThreshold <= 100) {
                    moistureThreshold = savedThreshold;
                    Serial.printf("[FIREBASE] Loaded moisture threshold: %d%%\n", moistureThreshold);
                } else {
                    Firebase.RTDB.setInt(&fbdo, "/greenhouse/controls/moisture_threshold", moistureThreshold);
                    Serial.printf("[FIREBASE] Set default moisture threshold: %d%%\n", moistureThreshold);
                }
            }
        } else {
            Firebase.RTDB.setInt(&fbdo, "/greenhouse/controls/moisture_threshold", moistureThreshold);
            Serial.printf("[FIREBASE] Set default moisture threshold: %d%%\n", moistureThreshold);
        }
        
        // Read auto-watering mode or set default (disabled)
        if (Firebase.RTDB.getInt(&fbdo, "/greenhouse/controls/auto_mode")) {
            if (fbdo.dataType() == "int") {
                autoWateringEnabled = (fbdo.intData() == 1);
                Serial.printf("[FIREBASE] Auto-watering mode: %s\n", autoWateringEnabled ? "ENABLED" : "DISABLED");
            }
        } else {
            Firebase.RTDB.setInt(&fbdo, "/greenhouse/controls/auto_mode", 0);
        }
        
        Firebase.RTDB.setInt(&fbdo, "/greenhouse/controls/water_15s", 0);
    } else {
        Serial.println("[ERROR] Firebase connection failed!");
        Serial.printf("[ERROR] Reason: %s\n\n", fbdo.errorReason().c_str());
        firebaseReady = false;
    }
}

// ========== SENSOR FUNCTIONS ==========

void readSensors() {
    Serial.println("[SENSOR] Reading sensors...");
    
    // Read BMP280
    sensors.temperature = bmp.readTemperature();
    sensors.pressure = bmp.readPressure() / 100.0F; // Convert to hPa
    sensors.altitude = bmp.readAltitude(1013.25);
    
    // Read BH1750
    sensors.lightLevel = lightMeter.readLightLevel();
    
    // Read Soil Moisture
    sensors.soilRaw = analogRead(SOIL_SENSOR_PIN);
    sensors.soilMoisture = mapSoilMoisture(sensors.soilRaw);
    
    // Print sensor values
    Serial.printf("  Temperature: %.2f °C\n", sensors.temperature);
    Serial.printf("  Pressure: %.2f hPa\n", sensors.pressure);
    Serial.printf("  Altitude: %.2f m\n", sensors.altitude);
    Serial.printf("  Light Level: %d lux\n", sensors.lightLevel);
    Serial.printf("  Soil Moisture: %d%% (Raw: %d)\n", sensors.soilMoisture, sensors.soilRaw);
    Serial.println();
}

int mapSoilMoisture(int raw) {
    // Map raw value to 0-100%
    // Lower raw value = more moisture (wet)
    // Higher raw value = less moisture (dry)
    int moisture = map(raw, SOIL_WET_VALUE, SOIL_DRY_VALUE, 100, 0);
    moisture = constrain(moisture, 0, 100);
    return moisture;
}

// ========== FIREBASE FUNCTIONS ==========

void uploadSensorsToFirebase() {
    if (!firebaseReady || !Firebase.ready()) {
        Serial.println("[WARN] Firebase not ready, skipping upload");
        return;
    }
    
    Serial.println("[FIREBASE] Uploading sensor data...");
    
    bool success = true;
    
    // Upload temperature
    if (!Firebase.RTDB.setFloat(&fbdo, "/greenhouse/sensors/temperature", sensors.temperature)) {
        Serial.printf("[ERROR] Temperature upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    // Upload pressure
    if (!Firebase.RTDB.setFloat(&fbdo, "/greenhouse/sensors/pressure", sensors.pressure)) {
        Serial.printf("[ERROR] Pressure upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    // Upload altitude
    if (!Firebase.RTDB.setFloat(&fbdo, "/greenhouse/sensors/altitude", sensors.altitude)) {
        Serial.printf("[ERROR] Altitude upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    // Upload light level
    if (!Firebase.RTDB.setInt(&fbdo, "/greenhouse/sensors/light", sensors.lightLevel)) {
        Serial.printf("[ERROR] Light upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    // Upload soil moisture percentage
    if (!Firebase.RTDB.setInt(&fbdo, "/greenhouse/sensors/soil_moisture", sensors.soilMoisture)) {
        Serial.printf("[ERROR] Soil moisture upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    // Upload soil raw value for debugging
    if (!Firebase.RTDB.setInt(&fbdo, "/greenhouse/sensors/soil_raw", sensors.soilRaw)) {
        Serial.printf("[ERROR] Soil raw upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    // Upload timestamp
    if (!Firebase.RTDB.setInt(&fbdo, "/greenhouse/sensors/timestamp", millis())) {
        Serial.printf("[ERROR] Timestamp upload failed: %s\n", fbdo.errorReason().c_str());
        success = false;
    }
    
    if (success) {
        Serial.println("[OK] All sensor data uploaded successfully!");
        // Flash RGB LED blue when uploading data
        flashRGBLED(0, 0, 255, 200);  // Blue flash for 200ms
    }
    
    // Upload to history every 30 seconds
    if (millis() - lastHistoryUpload >= HISTORY_UPLOAD_INTERVAL) {
        lastHistoryUpload = millis();
        String historyPath = "/greenhouse/history/" + String(millis());
        
        Firebase.RTDB.setInt(&fbdo, historyPath + "/moisture", sensors.soilMoisture);
        Firebase.RTDB.setFloat(&fbdo, historyPath + "/temperature", sensors.temperature);
        Firebase.RTDB.setFloat(&fbdo, historyPath + "/pressure", sensors.pressure);
        Firebase.RTDB.setInt(&fbdo, historyPath + "/timestamp", millis());
        
        Serial.println("[HISTORY] Data saved to Firebase");
        // Flash RGB LED blue when uploading history
        flashRGBLED(0, 0, 255, 200);  // Blue flash for 200ms
    }
}

// Check Firebase for remote commands
void checkFirebaseCommands() {
    if (!firebaseReady || !Firebase.ready()) {
        return;
    }
    
    // Check auto-watering mode
    if (Firebase.RTDB.getInt(&fbdo, "/greenhouse/controls/auto_mode")) {
        if (fbdo.dataType() == "int") {
            bool newAutoMode = (fbdo.intData() == 1);
            if (newAutoMode != autoWateringEnabled) {
                autoWateringEnabled = newAutoMode;
                Serial.printf("[FIREBASE] Auto-watering mode: %s\n", autoWateringEnabled ? "ENABLED" : "DISABLED");
            }
        }
    }
    
    // Check moisture threshold
    if (Firebase.RTDB.getInt(&fbdo, "/greenhouse/controls/moisture_threshold")) {
        if (fbdo.dataType() == "int") {
            int newThreshold = fbdo.intData();
            if (newThreshold > 0 && newThreshold <= 100 && newThreshold != moistureThreshold) {
                moistureThreshold = newThreshold;
                Serial.printf("[FIREBASE] Moisture threshold updated to: %d%%\n", moistureThreshold);
            }
        }
    }
    
    // Check manual pump trigger (15 seconds)
    if (Firebase.RTDB.getInt(&fbdo, "/greenhouse/controls/water_15s")) {
        if (fbdo.dataType() == "int") {
            int trigger = fbdo.intData();
            if (trigger == 1 && !pumpTimerActive) {
                Serial.println("[MANUAL] Starting pump for 15 seconds");
                startPumpTimer(15000);
                // Reset trigger
                Firebase.RTDB.setInt(&fbdo, "/greenhouse/controls/water_15s", 0);
            }
        }
    }
    
    // Check instant water pump control (for compatibility)
    // IMPORTANT: Ignore manual pump commands when timer is active
    if (!pumpTimerActive && Firebase.RTDB.getInt(&fbdo, "/greenhouse/controls/water_pump")) {
        if (fbdo.dataType() == "int") {
            int pumpCommand = fbdo.intData();
            bool newState = (pumpCommand == 1);
            
            if (newState != relayState) {
                controlRelay(newState);
                Serial.printf("[FIREBASE] Water pump command received: %s\n", newState ? "ON" : "OFF");
            }
        }
    }
}

// ========== RELAY CONTROL ==========

void controlRelay(bool state) {
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
    relayState = state;
    
    // Update Firebase with actual relay state
    if (firebaseReady && Firebase.ready()) {
        Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/relay_state", state ? 1 : 0);
    }
    
    Serial.printf("[RELAY] Water pump: %s\n", state ? "ON" : "OFF");
}

// ========== WIFI RESET BUTTON ==========

void checkButtonForReset() {
    static unsigned long lastDebounceTime = 0;
    int buttonState = digitalRead(BOOT_BUTTON_PIN);
    
    // Check if button is pressed (active LOW)
    if (buttonState == LOW) {
        if (!buttonPressed) {
            // Button just pressed
            buttonPressed = true;
            buttonPressStart = millis();
            lastDebounceTime = millis();
        } else {
            // Button held down
            unsigned long holdDuration = millis() - buttonPressStart;
            
            // Reset WiFi if held for 5 seconds
            if (holdDuration >= BUTTON_HOLD_TIME) {
                Serial.println("\n[RESET] BOOT button held for 5 seconds! Resetting WiFi...");
                wm.resetSettings();
                Serial.println("[RESET] WiFi credentials cleared. Restarting ESP32...");
                delay(500);
                ESP.restart();
            }
        }
    } else {
        // Button released
        if (buttonPressed) {
            buttonPressed = false;
        }
    }
}

// ========== PUMP TIMER FUNCTIONS ==========

void startPumpTimer(unsigned long duration) {
    pumpTimerActive = true;
    pumpStartTime = millis();
    pumpDuration = duration;
    controlRelay(true);
    
    // Update Firebase
    if (firebaseReady && Firebase.ready()) {
        Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/pump_timer_remaining", duration / 1000);
    }
}

void stopPumpTimer() {
    pumpTimerActive = false;
    controlRelay(false);
    Serial.println("[TIMER] Pump timer finished, stopping pump");
    
    // Update Firebase
    if (firebaseReady && Firebase.ready()) {
        Firebase.RTDB.setInt(&fbdo, "/greenhouse/status/pump_timer_remaining", 0);
    }
}

// ========== RGB LED FUNCTIONS ==========

void flashRGBLED(uint8_t r, uint8_t g, uint8_t b, int duration) {
    rgbLED.setPixelColor(0, rgbLED.Color(r, g, b));
    rgbLED.show();
    delay(duration);
    rgbLED.setPixelColor(0, rgbLED.Color(0, 0, 0));  // Turn off
    rgbLED.show();
}

// ========== MAIN LOOP ==========
