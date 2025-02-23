#include "display.h"
#include "hardware/adc.h"
#include "joystick/joystick.h"
#include <stdio.h>

// Variáveis do joystick
uint16_t vry_value = 0;

// Variáveis, constantes e funções para displaay oled
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

uint8_t ssd[ssd1306_buffer_length];

const int joystick_y = 0;

// Variáveis para apresentar dados no display
char temperature_char[6];
char soil_humidity_char[6];
char *temperature_text = "%d C";
int temperature_ant = 0;
int soil_humidity_ant = 0;

// Variáveis para permitir visualização do display ao entrar,
// independente do valor apresentado ser igual ao mostrado anteriomente ao sair
// da tela de apresentação da informação
bool motor_state = 1;
bool enter_motor_screen = 1;
bool enter_soil_screen = 1;

// Texto do menu
char *text[] = {
    "1  Temperatura   ",
    "2  Bomba dagua   ",
    "3  Umidade solo  "};

void configure_display() // Configura os pinos do display e a comunicaçã I2C
{
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();

    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void clear_display() // Limpa o display
{
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void init_system_message() // Mensagem enquanto o wifi conecta
{
    ssd1306_draw_string(ssd, 4, 8, "   Iniciando");
    ssd1306_draw_string(ssd, 4, 25, "    Sistema");
    render_on_display(ssd, &frame_area);
}

void draw_line_select(int y0, int y1) // Desenha a linha de seleção
{
    ssd1306_draw_line(ssd, 0, y0, 127, y0, true);   // primeira linha
    ssd1306_draw_line(ssd, 127, y1, 0, y1, true);   // segunda linha
    ssd1306_draw_line(ssd, 0, y0, 0, y1, true);     // primeira coluna
    ssd1306_draw_line(ssd, 127, y1, 127, y0, true); // segunda coluna
}

void draw_menu(int y0, int y1) // Desenha o menu
{

    clear_display(ssd, frame_area);

    ssd1306_draw_string(ssd, 4, 8, text[0]);
    ssd1306_draw_string(ssd, 4, 25, text[1]);
    ssd1306_draw_string(ssd, 4, 45, text[2]);
    draw_line_select(y0, y1);
    render_on_display(ssd, &frame_area);
}

void move_line_select(int *y0, int *y1, int *op) // Move a linha de seleção
{
    adc_select_input(joystick_y);
    sleep_us(2);
    vry_value = adc_read();
    if (vry_value < 95)
    {
        *y0 += 16;
        *y1 += 16;
        *op += 1;
        if (*y1 > 51)
        {
            *y0 = 5;
            *y1 = 18;
            *op = 1;
        }
        draw_menu(*y0, *y1);
    }
    else if (vry_value > 4000)
    {
        *y0 -= 16;
        *y1 -= 16;
        *op -= 1;
        if (*y0 < 5)
        {
            *y0 = 37;
            *y1 = 50;
            *op = 3;
        };
        draw_menu(*y0, *y1);
    }
}

void temperature_screen(int temperature) // Tela de temperatura
{

    if (temperature != temperature_ant) // Impede que a tela fique piscando
    {
        clear_display(ssd, frame_area);
        ssd1306_draw_string(ssd, 4, 8, "Temperatura");
        ssd1306_draw_string(ssd, 4, 25, "Atual: ");
        snprintf(temperature_char, sizeof(temperature_char), temperature_text, temperature);
        ssd1306_draw_string(ssd, 60, 25, temperature_char);
        render_on_display(ssd, &frame_area);
        temperature_ant = temperature;
    }
}

void water_bomb_screen(bool on_motor) // Tela de bomba d'agua
{
    if (enter_motor_screen || motor_state != on_motor) // Impede que a tela fique piscando, mas entre na tela menos uma vez quando o valor está igual
    {
        clear_display(ssd, frame_area);
        ssd1306_draw_string(ssd, 4, 8, "Bomba dagua");

        if (on_motor)
            ssd1306_draw_string(ssd, 4, 25, "Ligada");
        else
            ssd1306_draw_string(ssd, 4, 25, "Desligada");

        render_on_display(ssd, &frame_area);
        enter_motor_screen = 0;
        motor_state = on_motor;
    }
}

void soil_humidity_screen(int soil_humidity) // Tela de umidade do solo
{
    if (enter_soil_screen || soil_humidity_ant != soil_humidity) // Impede que a tela fique piscando, mas entre na tela menos uma vez quando o valor está igual
    {
        clear_display(ssd, frame_area);
        ssd1306_draw_string(ssd, 4, 8, "Umidade do solo");
        ssd1306_draw_string(ssd, 4, 25, "Atual: ");
        snprintf(soil_humidity_char, sizeof(soil_humidity_char), "%d", soil_humidity);
        ssd1306_draw_string(ssd, 60, 25, soil_humidity_char);
        render_on_display(ssd, &frame_area);
        enter_soil_screen = 0;
        soil_humidity_ant = soil_humidity;
    }
}

void back_menu(int op) // Retorna ao menu
{
    if (op == 1)
    {
        draw_menu(5, 18);
    }
    else if (op == 2)
    {
        draw_menu(21, 34);
    }
    else if (op == 3)
    {
        draw_menu(37, 50);
    }
    enter_motor_screen = 1;
    enter_soil_screen = 1;
    temperature_ant = 0;
}