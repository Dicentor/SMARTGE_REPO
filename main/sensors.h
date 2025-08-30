```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>


struct AirReading {
    float temperatureC = NAN;
    float humidity = NAN;
};


struct SoilReading {
    float fraction = NAN; // 0.0–1.0
    int raw = 0; // ADC Rohwert
};


struct AllReadings {
    AirReading air;
    SoilReading soil[3];
    bool npnActive = false;
};


void sensors_init();
AllReadings sensors_read_all();
```


---