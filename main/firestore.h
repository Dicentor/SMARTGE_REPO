```cpp
#pragma once
#include <string>
#include "sensors.h"
#include "config.h"


// Initialisiert Firebase Auth (anonyme Anmeldung) und merkt sich Tokens
bool firebase_auth_init();


// Sendet Telemetrie an Firestore: telemetry/{DEVICE_ID}
bool firestore_push(const AllReadings& r);


// Liest Konfiguration aus Firestore: configs/{DEVICE_ID} und schreibt in cfg
bool firestore_pull(DeviceConfig& cfg);
```