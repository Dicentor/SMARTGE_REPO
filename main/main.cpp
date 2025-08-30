```cpp
#include "esp_log.h"
#include "nvs_flash.h"
#include "config.h"
#include "sensors.h"
#include "relays.h"
#include "leds.h"
#include "firestore.h"


extern "C" void wifi_init_sta();


static const char* TAG = "main";


static DeviceConfig g_cfg; // wird per Firestore überschrieben


extern "C" void app_main()
{
ESP_ERROR_CHECK(nvs_flash_init());


leds_init(PIN_LED_RING, LED_COUNT);
leds_set_status(StatusLED::BOOT);


// WLAN hochfahren
leds_set_status(StatusLED::WIFI_CONNECTING);
wifi_init_sta();
leds_set_status(StatusLED::WIFI_OK);


// Firebase / Firestore Auth
if (firebase_auth_init()) {
leds_set_status(StatusLED::CLOUD_OK);
} else {
leds_set_status(StatusLED::ERROR);
}


// Hardware
sensors_init();
relays_init();


// Erste Konfig aus Cloud holen (optional ignorieren, wenn fehlgeschlagen)
firestore_pull(g_cfg);


// Zustandsvariablen für Bewässerung
uint64_t lastWaterMs[3] = {0,0,0};


uint64_t lastPush = 0;
uint64_t lastPull = 0;


while (true) {
uint64_t now = (uint64_t) (esp_timer_get_time()/1000ULL);


auto readings = sensors_read_all();


// Einfache Regel: pro Kanal, wenn Feuchte < Schwellwert und Cooldown vorbei -> Pumpe g_cfg.waterMs[i]
for (int i=0;i<3;i++) {
if (readings.soil[i].fraction < g_cfg.soilThreshold[i]) {
if (now - lastWaterMs[i] > g_cfg.cooldownMs) {
ESP_LOGI(TAG, "Channel %d watering for %d ms (soil=%.2f<thr=%.2f)", i+1, g_cfg.waterMs[i], readings.soil[i].fraction, g_cfg.soilThreshold[i]);
leds_set_status(i==0?StatusLED::WATERING1: i==1?StatusLED::WATERING2:StatusLED::WATERING3);
set_pump(i, true);
// Progressanzeige am LED-Ring
const int steps = 20; // 20 Updates während Laufzeit
for (int s=0; s<steps; ++s) {
float p = (float)(s+1)/steps;
leds_show_progress(i, p);
vTaskDelay(pdMS_TO_TICKS(g_cfg.waterMs[i]/steps));
}
set_pump(i, false);
leds_set_status(StatusLED::CLOUD_OK);
lastWaterMs[i] = now;
}
}
}


// Lüfter manuell
if (g_cfg.fanManual) set_fan(g_cfg.fanOn);
else {
// einfache Automatik: >75% rF -> Lüfter EIN, sonst AUS
bool fan = (!std::isnan(readings.air.humidity) && readings.air.humidity > 75.0f);
set_fan(fan);
}


// Telemetrie pushen
if (now - lastPush > FIRESTORE_PUSH_MS) {
firestore_push(readings);
lastPush = now;
}
// Konfig pullen
if (now - lastPull > FIRESTORE_PULL_MS) {
firestore_pull(g_cfg);
lastPull = now;
}


vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
}
}
```