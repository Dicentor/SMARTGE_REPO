```cpp
#include "dht22.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"


// Einfache Bitbang-Implementierung für DHT22
// Timing basierend auf Datenblatt (ca.). Läuft auf 80MHz APB.


static inline void delay_us(uint32_t us) {
    esp_rom_delay_us(us);
}


std::optional<DHT22Reading> dht22_read(int gpio)
{
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << gpio);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // interner Pullup hilft
    gpio_config(&io_conf);


    // Startsignal: 1–10ms low
    gpio_set_level((gpio_num_t)gpio, 0);
    delay_us(1200);
    // Wechsel auf Input, warten auf Antwort
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);


    // Warte auf Sensor-Pegelwechsel (Antwort)
    uint32_t timeout = 0;
    while (gpio_get_level((gpio_num_t)gpio) == 1) { if (++timeout > 10000) return std::nullopt; delay_us(1); }
    while (gpio_get_level((gpio_num_t)gpio) == 0) { if (++timeout > 20000) return std::nullopt; delay_us(1); }
    while (gpio_get_level((gpio_num_t)gpio) == 1) { if (++timeout > 20000) return std::nullopt; delay_us(1); }


    // 40 Bits lesen (16 Hum, 16 Temp, 8 Checksum)
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; ++i) {
        // Warte auf Start des Bits (LOW ~50us)
        timeout = 0;
        while (gpio_get_level((gpio_num_t)gpio) == 0) { if (++timeout > 10000) return std::nullopt; delay_us(1); }
        // HIGH-Phase messen
        uint32_t t = 0;
        while (gpio_get_level((gpio_num_t)gpio) == 1) { if (++t > 20000) return std::nullopt; delay_us(1); }
        // Wenn HIGH > ~28us -> Bit=1, sonst 0
        uint8_t bit = (t > 40) ? 1 : 0; // konservativ
        data[i / 8] <<= 1;
        data[i / 8] |= bit;
    }


    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return std::nullopt;


    int16_t raw_h = (data[0] << 8) | data[1];
    int16_t raw_t = (data[2] << 8) | data[3];


    float humidity = raw_h / 10.0f;
    float temp = (raw_t & 0x8000) ? -((raw_t & 0x7FFF) / 10.0f) : (raw_t / 10.0f);


    return DHT22Reading{temp, humidity};
}
```