```cpp
#include "sensors.h"
#include "config.h"
#include "dht22.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include <algorithm>


static adc_oneshot_unit_handle_t adc1_handle;


static float map_soil_percent(int raw, int dry, int wet) {
    int clamped = std::clamp(raw, std::min(dry, wet), std::max(dry, wet));
    float frac = (float)(clamped - dry) / (float)(wet - dry); // wet < dry -> negativ
    // Wenn wet < dry, invertiert sich das Vorzeichen -> wir drehen auf 0..1
    float pct = 1.0f - std::clamp(frac, -1.0f, 1.0f);
    return std::clamp(pct, 0.0f, 1.0f);
}


void sensors_init()
{
    // ADC One-Shot für ADC1 Kanäle
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);


    auto cfg_channel = [](adc_channel_t ch){
        adc_oneshot_chan_cfg_t cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11 // bis ~3.6V
        };
        adc_oneshot_config_channel(adc1_handle, ch, &cfg);
    };


    cfg_channel((adc_channel_t)ADC_CH_SOIL1);
    cfg_channel((adc_channel_t)ADC_CH_SOIL2);
    cfg_channel((adc_channel_t)ADC_CH_SOIL3);


    // NPN Digital In
    gpio_config_t io = {};
    io.mode = GPIO_MODE_INPUT;
    io.pin_bit_mask = 1ULL << PIN_NPN_SENSOR;
    io.pull_up_en = GPIO_PULLUP_ENABLE; // NPN-Open-Collector -> Pullup nötig
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io);
}


AllReadings sensors_read_all()
{
    AllReadings r;


    // DHT22
    if (auto d = dht22_read(PIN_DHT22)) {
        r.air.temperatureC = d->temperatureC;
        r.air.humidity = d->humidity;
    }


    int raw1=0, raw2=0, raw3=0;
    adc_oneshot_read(adc1_handle, (adc_channel_t)ADC_CH_SOIL1, &raw1);
    adc_oneshot_read(adc1_handle, (adc_channel_t)ADC_CH_SOIL2, &raw2);
    adc_oneshot_read(adc1_handle, (adc_channel_t)ADC_CH_SOIL3, &raw3);


    r.soil[0].raw = raw1;
    r.soil[1].raw = raw2;
    r.soil[2].raw = raw3;


    r.soil[0].fraction = map_soil_percent(raw1, SOIL1_RAW_DRY, SOIL1_RAW_WET);
    r.soil[1].fraction = map_soil_percent(raw2, SOIL2_RAW_DRY, SOIL2_RAW_WET);
    r.soil[2].fraction = map_soil_percent(raw3, SOIL3_RAW_DRY, SOIL3_RAW_WET);


    r.npnActive = (gpio_get_level((gpio_num_t)PIN_NPN_SENSOR) == 0) ? true : false; // NPN zieht nach GND


    return r;
}
```