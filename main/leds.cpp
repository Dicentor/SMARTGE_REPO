```cpp
#include "leds.h"
#include "led_strip.h"
#include "config.h"
#include <algorithm>


static led_strip_handle_t strip;
static int g_led_count = 0;


static void set_all(uint8_t r, uint8_t g, uint8_t b) {
    for (int i=0;i<g_led_count;++i) led_strip_set_pixel(strip, i, r,g,b);
    led_strip_refresh(strip);
}


void leds_init(int gpio, int led_count)
{
    g_led_count = led_count;


    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = (uint32_t)led_count,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags = { .invert_out = 0 }
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 0,
        .flags = { .with_dma = 0 }
    };
    led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);
    led_strip_clear(strip);
}


void leds_set_status(StatusLED st)
{
    switch (st) {
        case StatusLED::BOOT: set_all(8,8,8); break; // weiß, dunkel
        case StatusLED::WIFI_CONNECTING: set_all(0,0,16); break; // blau
        case StatusLED::WIFI_OK: set_all(0,16,0); break; // grün
        case StatusLED::CLOUD_OK: set_all(0,16,16); break; // cyan
        case StatusLED::WATERING1: set_all(16,0,0); break; // rot
        case StatusLED::WATERING2: set_all(16,8,0); break; // orange
        case StatusLED::WATERING3: set_all(16,0,16); break; // magenta
        case StatusLED::ERROR: set_all(16,0,0); break; // rot hell
    }
}


void leds_show_progress(int channel, float p)
{
    // Einfach: erster n-Anteil leuchtet grün, Rest aus; je Kanal ein anderes Farbmuster
    p = std::clamp(p, 0.0f, 1.0f);
    int lit = (int)(p * g_led_count + 0.5f);
    for (int i=0;i<g_led_count;i++) {
        uint8_t r=0,g=0,b=0;
        if (i<lit) {
            switch(channel){
                case 0: g=20; break; // grün
                case 1: r=20; g=10; break; // gelblich
                case 2: r=20; b=20; break; // pink
                }
            }
            led_strip_set_pixel(strip, i, r,g,b);
    }
    led_strip_refresh(strip);
}
```