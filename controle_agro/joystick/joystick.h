#ifndef JOYSTICK_H
#define JOYSTICK_H

extern const int VRX;           // Pino de leitura do eixo X do joystick (conectado ao ADC)
extern const int VRY;           // Pino de leitura do eixo Y do joystick (conectado ao ADC)
extern const int ADC_CHANNEL_0; // Canal ADC para o eixo X do joystick
extern const int ADC_CHANNEL_1; // Canal ADC para o eixo Y do joystick
extern const int SW;            // Pino de leitura do botão do joystick

void configure_joystick();
#endif