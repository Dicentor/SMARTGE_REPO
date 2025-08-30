```cpp
#pragma once
#include <cstdint>


enum class StatusLED {
    BOOT,
    WIFI_CONNECTING,
    WIFI_OK,
    CLOUD_OK,
    WATERING1,
    WATERING2,
    WATERING3,
    ERROR
};


void leds_init(int gpio, int led_count);
void leds_set_status(StatusLED st);
void leds_show_progress(int ledIndex /*0..2*/, float progress01);
```