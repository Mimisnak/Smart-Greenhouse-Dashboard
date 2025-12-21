# 🌱 Smart Greenhouse Web Dashboard

Όμορφο web interface για το ESP32-S3 Smart Greenhouse σου!

## ✨ Χαρακτηριστικά

- ✅ **Real-time δεδομένα** από Firebase
- ✅ **Έλεγχος αντλίας** με ένα κλικ
- ✅ **Responsive design** (δουλεύει σε κινητά)
- ✅ **Όμορφο UI** με animations
- ✅ **Progress bar** για υγρασία χώματος
- ✅ **Connection status indicator**
- ✅ **Countdown timer** για επόμενη ανανέωση

## 🚀 Πώς να το Ανεβάσεις στο GitHub Pages

### Βήμα 1: Δημιούργησε GitHub Repository

1. Πήγαινε στο [GitHub](https://github.com)
2. Κάνε κλικ στο **"New Repository"**
3. Όνομα: `greenhouse-dashboard` (ή ότι θες)
4. Επέλεξε **Public**
5. Πάτα **"Create Repository"**

### Βήμα 2: Ανέβασε τα Αρχεία

Στο VS Code terminal (μέσα στον φάκελο `web-dashboard`):

```bash
cd "c:\Users\mimis\Desktop\IoT PTYXIAKH ASP32\web-dashboard"
git init
git add .
git commit -m "Initial commit: Smart Greenhouse Dashboard"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/greenhouse-dashboard.git
git push -u origin main
```

### Βήμα 3: Ενεργοποίησε GitHub Pages

1. Στο GitHub repository, πήγαινε στο **Settings**
2. Πήγαινε στο **Pages** (αριστερά)
3. Στο **Source** διάλεξε: **main branch**
4. Πάτα **Save**
5. Περίμενε 1-2 λεπτά

### Βήμα 4: Δες το Live!

Το site σου θα είναι εδώ:
```
https://YOUR-USERNAME.github.io/greenhouse-dashboard/
```

## ⚙️ Ρυθμίσεις Firebase

Άνοιξε το `index.html` και **ΑΛΛΑΞΕ** τα Firebase credentials (γραμμές 109-117):

```javascript
const firebaseConfig = {
    apiKey: "ΤΟ-ΔΙΚΟ-ΣΟΥ-API-KEY",
    authDomain: "ΤΟ-ΔΙΚΟ-ΣΟΥ-PROJECT.firebaseapp.com",
    databaseURL: "https://ΤΟ-ΔΙΚΟ-ΣΟΥ-PROJECT-default-rtdb.firebaseio.com",
    projectId: "ΤΟ-ΔΙΚΟ-ΣΟΥ-PROJECT-ID",
    storageBucket: "ΤΟ-ΔΙΚΟ-ΣΟΥ-PROJECT.firebasestorage.app",
    messagingSenderId: "ΤΟ-ΔΙΚΟ-ΣΟΥ-SENDER-ID",
    appId: "ΤΟ-ΔΙΚΟ-ΣΟΥ-APP-ID"
};
```

**Που τα βρίσκω;**
- Firebase Console → ⚙️ Settings → Project Settings
- Scroll κάτω → "Your apps" → Web app
- Αντίγραψε τα credentials

## 📊 Firebase Database Rules

Για να δουλέψει σωστά, βάλε αυτά τα rules στο Firebase:

```json
{
  "rules": {
    "greenhouse": {
      ".read": true,
      ".write": true
    }
  }
}
```

**Πώς;**
1. Firebase Console → Realtime Database
2. **Rules** tab
3. Κάνε paste τα παραπάνω
4. **Publish**

## 🎨 Τι Δείχνει το Dashboard

- 🌡️ **Θερμοκρασία** (°C)
- 🔴 **Πίεση** (hPa)
- ⛰️ **Υψόμετρο** (m)
- 💡 **Φωτισμός** (lux)
- 💧 **Υγρασία Χώματος** (% με progress bar)
- 🚰 **Αντλία Νερού** (ON/OFF control)

## 🔄 Real-time Updates

- Δεδομένα ανανεώνονται **αυτόματα** από Firebase
- Countdown timer δείχνει πότε θα έρθουν νέα δεδομένα (30s)
- Connection indicator δείχνει αν είναι συνδεδεμένο

## 📱 Mobile Friendly

Το dashboard είναι **responsive** και δουλεύει τέλεια σε:
- 💻 Desktop
- 📱 Κινητά
- 📱 Tablets

## 🆘 Troubleshooting

**Δεν βλέπω δεδομένα:**
- Έλεγξε αν το ESP32 στέλνει στο Firebase
- Έλεγξε τα Firebase credentials στο `index.html`
- Έλεγξε τα Database Rules

**Δεν ελέγχει την αντλία:**
- Έλεγξε τα Database Rules (πρέπει `.write: true`)
- Άνοιξε το Console του browser (F12) για errors

**Connection status: Offline:**
- Έλεγξε το Firebase databaseURL
- Έλεγξε αν το ESP32 είναι online

## 📄 Αρχεία

```
web-dashboard/
├── index.html      # Το κύριο HTML
├── style.css       # Τα styles
└── README.md       # Αυτό το αρχείο
```

## 🎉 Έτοιμο!

Τώρα έχεις ένα **professional dashboard** για το greenhouse σου!

Καλή επιτυχία! 🚀🌱
