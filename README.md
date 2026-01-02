# Smart Greenhouse Dashboard & ESP32 Firmware

## Περιγραφή

Έξυπνο θερμοκήπιο με ESP32-S3, αισθητήρες, Firebase και web dashboard για παρακολούθηση και αυτόματο πότισμα.

## Χαρακτηριστικά

- ESP32-S3 DevKitC-1
- Αισθητήρες: BMP280 (Θερμοκρασία, Πίεση), BH1750 (Φωτεινότητα), Soil Moisture
- Relay για αντλία νερού
- RGB LED για ειδοποιήσεις
- WiFiManager για εύκολη σύνδεση
- Firebase Realtime Database για αποθήκευση και ιστορικό
- Web Dashboard με Chart.js για γραφήματα
- Αυτόματο πότισμα με όριο υγρασίας
- Αυτόματη διαγραφή παλιών δεδομένων ιστορικού (24h)

## Hardware

- ESP32-S3 DevKitC-1
- BMP280: SDA=16, SCL=17, I2C Addr=0x76
- BH1750: SDA=8, SCL=9, I2C Addr=0x23
- Soil Sensor: GPIO 4 (Analog)
- Relay: GPIO 5
- RGB LED: GPIO 48
- BOOT Button: GPIO 0

## Αρχεία

- `src/main.cpp`: Firmware ESP32
- `index.html`, `style.css`: Web Dashboard
- `platformio.ini`: Ρυθμίσεις PlatformIO & βιβλιοθήκες
- `firebase-rules.json`: Ασφαλείς κανόνες Firebase
- `SECURITY-SETUP.md`: Οδηγίες ασφαλείας Firebase

## Setup ESP32

1. Εγκατάσταση PlatformIO
2. Ρύθμιση WiFi μέσω WiFiManager (AP: Greenhouse_Connect)
3. Εισαγωγή Firebase credentials στο main.cpp
4. Ανέβασμα firmware με PlatformIO

## Web Dashboard

- Ανοίξτε το `index.html` σε browser
- Δείτε live δεδομένα, ιστορικό & γραφήματα
- Ελέγξτε αντλία και όρια υγρασίας

## Ασφάλεια Firebase

- Ενεργοποιήστε Anonymous Authentication
- Εφαρμόστε κανόνες από `firebase-rules.json`
- Δείτε οδηγίες στο `SECURITY-SETUP.md`

## Συνεισφορά & Άδεια

- Copyright © 2026 developed by mimis.dev
- Για εκπαιδευτική χρήση

---

Για απορίες/βελτιώσεις: [mimis.dev](mailto:info@mimis.dev)
