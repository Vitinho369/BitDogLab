#include "button.h"
#include <stdio.h>
#include "pico/stdlib.h"

const uint BUTTON_A_PIN = 5; // Botão A no GPIO 5

void configure_buttons() // Função para configurar o botão
{
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
}