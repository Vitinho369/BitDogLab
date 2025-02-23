#include "motor.h"

const int led_pin = 12;

void configure_motor() // Função para configurar o led
{
    gpio_init(led_pin);
    gpio_set_slew_rate(led_pin, GPIO_SLEW_RATE_SLOW);
    gpio_set_dir(led_pin, true);
}

void set_motor(int on_motor) // Função para ligar/desligar o led
{
    gpio_put(led_pin, on_motor);
}
