#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LEDR 12
#define LEDG 11
#define LEDB 13

void led_rgb_put(bool r, bool g, bool b)
{
    gpio_put(LEDR, r);
    gpio_put(LEDG, g);
    gpio_put(LEDB, b);
}

int main()
{
    stdio_init_all();

    gpio_init(LEDR);
    gpio_init(LEDG);
    gpio_init(LEDB);

    gpio_set_slew_rate(LEDR, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(LEDG, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(LEDB, GPIO_SLEW_RATE_SLOW);

    gpio_set_dir(LEDR, true);
    gpio_set_dir(LEDG, true);
    gpio_set_dir(LEDB, true);

    while (true)
    {
        printf("Testando led rgb");
        led_rgb_put(true, false, false);
        sleep_ms(500);
        led_rgb_put(false, true, false);
        sleep_ms(500);
        led_rgb_put(false, false, true);
        sleep_ms(500);
        led_rgb_put(true, true, true);
        sleep_ms(500);
    }
}