```cpp
#pragma once
#include <cstdint>
#include <array>


// ==== Pinbelegung (bei Bedarf an dein Board/Layout anpassen) ====
// Achtung: GPIO34 ist input-only (gut für ADC), Relaisausgänge nutzen GPIOs mit Output-Fähigkeit.


// Sensoren
constexpr int PIN_DHT22 = 4; // Digitaler 1-Wire DHT22 Datenpin
constexpr int PIN_NPN_SENSOR = 5; // Digitaler Eingang (NPN, ggf. Pullup aktivieren)


// ADC Bodenfeuchte (Spannungsausgänge): ADC1 Kanäle
// GPIO32 -> ADC1_CH4, GPIO33 -> ADC1_CH5, GPIO34 -> ADC1_CH6
constexpr int ADC_CH_SOIL1 = 4; // ADC1_CH4 (GPIO32)
constexpr int ADC_CH_SOIL2 = 5; // ADC1_CH5 (GPIO33)
constexpr int ADC_CH_SOIL3 = 6; // ADC1_CH6 (GPIO34)


// LED-Ring (WS2812/NeoPixel-artig)
constexpr int PIN_LED_RING = 18; // RMT fähiger GPIO
constexpr int LED_COUNT = 8; // 8 RGB-LEDs


// Relais (3 Pumpen + 1 Lüfter)
constexpr int PIN_RELAY_PUMP1 = 23;
constexpr int PIN_RELAY_PUMP2 = 22;
constexpr int PIN_RELAY_PUMP3 = 21;
constexpr int PIN_RELAY_FAN = 19;


// Falls deine Relais-Module "active-low" sind, hier invertieren
constexpr bool RELAY_ACTIVE_LOW = true;


// ==== Steuerung / Defaults ====
struct DeviceConfig {
    // Schaltschwellen Bodenfeuchte (0.0–1.0 als Prozentanteil 0–100%)
    float soilThreshold[3] = {0.35f, 0.35f, 0.35f};
    // Gießmenge als Laufzeit in Millisekunden
    int32_t waterMs[3] = {3000, 3000, 3000};
    // optional: manuelle Lüftersteuerung aus Firestore
    bool fanManual = false;
    bool fanOn = false;
    // Cooldown zwischen Gießzyklen pro Kanal (ms), damit nicht ständig getriggert wird
    uint32_t cooldownMs = 30 * 60 * 1000; // 30 Minuten
};


// Mapping ADC-Rohwert -> Bodenfeuchte-"Prozent" (0.0–1.0)
// Kalibrierung: passe DRY/WET auf deine Sensoren an (z.B. trocken ~ 2800, nass ~ 1200)
constexpr int ADC_MAX_RAW = 4095; // 12 Bit ADC
constexpr int SOIL1_RAW_DRY = 3000;
constexpr int SOIL1_RAW_WET = 1200;
constexpr int SOIL2_RAW_DRY = 3000;
constexpr int SOIL2_RAW_WET = 1200;
constexpr int SOIL3_RAW_DRY = 3000;
constexpr int SOIL3_RAW_WET = 1200;


// Mess-/Syncintervalle
constexpr uint32_t SAMPLE_INTERVAL_MS = 5000; // Sensorlesen
constexpr uint32_t FIRESTORE_PUSH_MS = 60000; // Telemetrie senden
constexpr uint32_t FIRESTORE_PULL_MS = 300000; // Konfig abrufen (5 min)
```