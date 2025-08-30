```cpp
#pragma once
#include <optional>


struct DHT22Reading {
    float temperatureC;
    float humidity;
};


// DHT22 Lese-Funktion (blocking ~ 2ms). Gibt std::nullopt bei Fehler.
std::optional<DHT22Reading> dht22_read(int gpio);
```