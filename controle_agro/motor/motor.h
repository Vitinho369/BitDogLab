#ifndef MOTOR_H
#define MOTOR_H
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

extern const int led_pin; // Pino do led

void configure_motor();
void set_motor(int on_motor);
#endif
