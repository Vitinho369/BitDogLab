#ifndef DISPLAY_H
#define DISPLAY_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

// Variáveis do joystick
extern uint16_t vry_value;

// Variáveis para display oled
extern const uint I2C_SDA;
extern const uint I2C_SCL;

extern struct render_area frame_area;
extern struct render_area temperature_area;

extern uint8_t ssd[ssd1306_buffer_length];
extern char *text[];
extern const int joystick_y;

extern char temperature_char[6];
extern char soil_humidity_char[6];
extern char *text_temperature;
extern int temperature_ant;
extern int soil_humidity_ant;
extern bool enter_soil_screen;
extern bool motor_state;

// Funções para display
void init_system_message();
void configure_display();
void clear_display();
void back_menu(int op);
void draw_menu(int y0, int y1);
void draw_line_select(int y0, int y1);
void move_line_select(int *y0, int *y1, int *op);
void temperature_screen(int temperature);
void water_bomb_screen(bool on_motor);
void soil_humidity_screen(int soil_humidity);

#endif // DISPLAY_H
