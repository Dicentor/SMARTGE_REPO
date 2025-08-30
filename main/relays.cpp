```cpp
#include "relays.h"
#include "driver/gpio.h"


static inline void write_relay(int gpio, bool on) {
    bool level = RELAY_ACTIVE_LOW ? !on : on;
    gpio_set_level((gpio_num_t)gpio, level);
}


void relays_init()
{
    gpio_config_t io = {};
    io.mode = GPIO_MODE_OUTPUT;
    io.intr_type = GPIO_INTR_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.pull_up_en = GPIO_PULLUP_DISABLE;


    io.pin_bit_mask = (1ULL<<PIN_RELAY_PUMP1) | (1ULL<<PIN_RELAY_PUMP2) | (1ULL<<PIN_RELAY_PUMP3) | (1ULL<<PIN_RELAY_FAN);
    gpio_config(&io);


    // Alle aus
    write_relay(PIN_RELAY_PUMP1, false);
    write_relay(PIN_RELAY_PUMP2, false);
    write_relay(PIN_RELAY_PUMP3, false);
    write_relay(PIN_RELAY_FAN, false);
    }


void set_pump(int index, bool on)
{
    switch(index){
        case 0: write_relay(PIN_RELAY_PUMP1, on); break;
        case 1: write_relay(PIN_RELAY_PUMP2, on); break;
        case 2: write_relay(PIN_RELAY_PUMP3, on); break;
    }
}


void set_fan(bool on)
{
    write_relay(PIN_RELAY_FAN, on);
}
```