#include "joystick.h"
#include "hardware/adc.h"

const int VRX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int VRY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick
const int SW = 22;           // Pino de leitura do botão do joystick

void configure_joystick() // Função para configurar o joystick
{
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);

    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW); // Ativa o pull-up no pino do botão para evitar flutuações
}