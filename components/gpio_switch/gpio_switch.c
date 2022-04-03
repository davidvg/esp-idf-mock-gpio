#include <stdio.h>

#include "driver/gpio.h"

#include "gpio_switch.h"

void configure_output(int pin)
{
    gpio_config_t cfg = 
    {
        .pin_bit_mask = 1ULL << (gpio_num_t)pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(pin, 0);
}