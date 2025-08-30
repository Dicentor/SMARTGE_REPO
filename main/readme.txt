```text
Vorbereitung Firestore / Firebase
---------------------------------
1) Firebase-Projekt anlegen, Firestore im "Native Mode" aktivieren.
2) Unter Authentication -> Sign-in method "Anonymous" aktivieren.
3) Unter Projekteinstellungen -> Allgemein -> Deine Apps -> Web-API-Key kopieren -> in secrets.h eintragen.
4) Firestore-Sicherheitsregeln für Tests lockern (nur zum Entwickeln!), z.B. erlauben, dass anonymous Nutzer read/write auf `configs/*` und `telemetry/*` haben. Für Produktion bitte restriktive Regeln + Auth-Konzept!
5) In der Collection `configs`, ein Dokument mit ID = DEVICE_ID anlegen (siehe secrets.h), z.B. Felder:
- soilThreshold: Array (Double) [0.35,0.35,0.35]
- waterMs: Array (Integer) [3000,3000,3000]
- fanManual: Boolean (false)
- fanOn: Boolean (false)


Build & Flash (ESP-IDF v5.x)
----------------------------
- Abhängigkeiten installieren, dann:
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor


Kalibrierung Bodenfeuchte
-------------------------
Passe in config.h die Konstanten SOIL*_RAW_DRY/WET an deine Sensoren an. Rohwerte siehst du im Monitor (füge ggf. Logs ein).


LED-Ring
--------
Die Implementierung nutzt die ESP-IDF `led_strip` (RMT) Komponente für WS2812-kompatible Ringe.


TLS / Zertifikate
-----------------
Trage in secrets.h eine gültige Root/Intermediate PEM-Kette für Google ein. Ohne korrektes Zertifikat schlagen HTTPS-Requests fehl.
```