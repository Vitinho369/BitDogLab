#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
static volatile int op = 1;
static volatile bool enable_joystick_led_mode = false;
static volatile bool enable_buzzer_mode = false;
static volatile bool enable_led_rgb_mode = false;

static absolute_time_t last_interrupt_time = 0; // Armazena o tempo do último acionamento

const uint LED = 12;                    // Pino do LED conectado
const uint16_t PERIOD_LED_RGB = 2000;   // Período do PWM (valor máximo do contador)
const float DIVIDER_PWM_LED_RGB = 16.0; // Divisor fracional do clock para o PWM
const uint16_t LED_STEP = 100;          // Passo de incremento/decremento para o duty cycle do LED
uint16_t led_level = 100;               // Nível inicial do PWM (duty cycle)
uint up_down = 1;

#define BUZZER_PIN 21

const int VRX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int VRY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick
const int SW = 22;           // Pino de leitura do botão do joystick

const int LED_B = 13;                    // Pino para controle do LED azul via PWM
const int LED_R = 11;                    // Pino para controle do LED vermelho via PWM
const float DIVIDER_PWM = 16.0;          // Divisor fracional do clock para o PWM
const uint16_t PERIOD = 4096;            // Período do PWM (valor máximo do contador)
uint16_t led_b_level, led_r_level = 0;   // Inicialização dos níveis de PWM para os LEDs
uint slice_led_b, slice_led_r;           // Variáveis para armazenar os slices de PWM correspondentes aos LEDs
uint16_t vrx_value, vry_value, sw_value; // Variáveis para armazenar os valores do joystick (eixos X e Y) e botão                                    // Chama a função de configuração

// Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

uint8_t ssd[ssd1306_buffer_length];

const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392};

// Duração das notas em milissegundos
const uint note_duration[] = {
    500,
    500,
    500,
    350,
    150,
    300,
    500,
    350,
    150,
    300,
    500,
    500,
    500,
    500,
    350,
    150,
    300,
    500,
    500,
    350,
    150,
    300,
    500,
    350,
    150,
    300,
    650,
    500,
    150,
    300,
    500,
    350,
    150,
    300,
    500,
    150,
    300,
    500,
    350,
    150,
    300,
    650,
    500,
    350,
    150,
    300,
    500,
    350,
    150,
    300,
    500,
    500,
    500,
    500,
    350,
    150,
    300,
    500,
    500,
    350,
    150,
    300,
    500,
    350,
    150,
    300,
    500,
    350,
    150,
    300,
    500,
    500,
    350,
    150,
    300,
    500,
    500,
    350,
    150,
    300,
};

char *text[] = {
    "1  Joystick Led   ",
    "2  Buzzer  ",
    "3  Led RGB  "};

void clear_display();
void draw_menu(int y0, int y1);
void draw_line_select(int y0, int y1);
void move_line_select();

void setup_joystick();
void gpio_callback(uint gpio, uint32_t events);
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value);
void setup_pwm_led(uint led, uint *slice, uint16_t level);

void pwm_init_buzzer(uint pin);
void play_tone(uint pin, uint frequency, uint duration_ms);

void setup_pwm();

void joystick_led_mode();
void buzzer_mode();
void led_rgb_mode();

int main()
{
    int y0 = 5, y1 = 18;
    stdio_init_all();
    setup_joystick();
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    setup_pwm_led(LED_B, &slice_led_b, led_b_level);
    setup_pwm_led(LED_R, &slice_led_r, led_r_level);

    pwm_init_buzzer(BUZZER_PIN);
    setup_pwm();

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    draw_menu(y0, y1);

    while (true)
    {

        if (op <= 3)
        {
            move_line_select(&y0, &y1, &op);
        }
        else
        {
            if (enable_joystick_led_mode)
            {
                joystick_led_mode();
            }
            else if (enable_buzzer_mode)
            {
                buzzer_mode();
            }
            else if (enable_led_rgb_mode)
            {
                led_rgb_mode();
            }
        }

        sleep_ms(100);
    }

    return 0;
}

void move_line_select(int *y0, int *y1, int *op)
{
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    vry_value = adc_read();
    if (vry_value < 20)
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

// Joystick led
void setup_joystick()
{
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);

    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW); // Ativa o pull-up no pino do botão para evitar flutuações
}

void setup_pwm_led(uint led, uint *slice, uint16_t level)
{
    gpio_set_function(led, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(led);
    pwm_set_clkdiv(*slice, DIVIDER_PWM);
    pwm_set_wrap(*slice, PERIOD);
    pwm_set_gpio_level(led, level);
    pwm_set_enabled(*slice, true);
}

void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value)
{
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vrx_value = adc_read();

    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vry_value = adc_read();
}

// Buzzer

void pwm_init_buzzer(uint pin)
{
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms)
{
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50);               // Pausa entre notas
}

// Led RGB
void setup_pwm()
{
    uint slice;
    gpio_set_function(LED, GPIO_FUNC_PWM);      // Configura o pino do LED para função PWM
    slice = pwm_gpio_to_slice_num(LED);         // Obtém o slice do PWM associado ao pino do LED
    pwm_set_clkdiv(slice, DIVIDER_PWM_LED_RGB); // Define o divisor de clock do PWM
    pwm_set_wrap(slice, PERIOD_LED_RGB);        // Configura o valor máximo do contador (período do PWM)
    pwm_set_gpio_level(LED, 0);                 // Define o nível inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true);               // Habilita o PWM no slice correspondente
}

// Display Oled
void clear_display()
{
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void draw_line_select(int y0, int y1)
{
    ssd1306_draw_line(ssd, 0, y0, 127, y0, true);   // primeira linha
    ssd1306_draw_line(ssd, 127, y1, 0, y1, true);   // segunda linha
    ssd1306_draw_line(ssd, 0, y0, 0, y1, true);     // primeira coluna
    ssd1306_draw_line(ssd, 127, y1, 127, y0, true); // segunda coluna
}

void draw_menu(int y0, int y1)
{

    clear_display(ssd, frame_area);
    // Desenha o menu
    ssd1306_draw_string(ssd, 4, 8, text[0]);
    ssd1306_draw_string(ssd, 4, 25, text[1]);
    ssd1306_draw_string(ssd, 4, 45, text[2]);
    draw_line_select(y0, y1);
    render_on_display(ssd, &frame_area);
}

// Modos de operação
void joystick_led_mode()
{
    joystick_read_axis(&vrx_value, &vry_value);

    pwm_set_gpio_level(LED_B, vrx_value);
    pwm_set_gpio_level(LED_R, vry_value);
}

void buzzer_mode()
{
    for (int i = 0; (i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]) && enable_buzzer_mode); i++)
    {

        if (star_wars_notes[i] == 0)
        {
            sleep_ms(note_duration[i]);
        }
        else
        {
            play_tone(BUZZER_PIN, star_wars_notes[i], note_duration[i]);
        }
    }
}

void led_rgb_mode()
{
    pwm_set_gpio_level(LED, led_level); // Define o nível atual do PWM (duty cycle)
    sleep_ms(1000);                     // Atraso de 1 segundo
    if (up_down)
    {
        led_level += LED_STEP; // Incrementa o nível do LED
        if (led_level >= PERIOD_LED_RGB)
            up_down = 0; // Muda direção para diminuir quando atingir o período máximo
    }
    else
    {
        led_level -= LED_STEP; // Decrementa o nível do LED
        if (led_level <= LED_STEP)
            up_down = 1; // Muda direção para aumentar quando atingir o mínimo
    }
}

void back_menu()
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
    led_level = 100;
    led_r_level = 0;
    led_b_level = 0;
    pwm_set_gpio_level(LED, 0);
    setup_pwm_led(LED_B, &slice_led_b, led_b_level); // Configura o PWM para o LED azul
    setup_pwm_led(LED_R, &slice_led_r, led_r_level); // Configura o PWM para o LED vermelho

    enable_joystick_led_mode = false;
    enable_buzzer_mode = false;
    enable_led_rgb_mode = false;
}

// Função de callback
void gpio_callback(uint gpio, uint32_t events)
{
    absolute_time_t current_time = get_absolute_time();
    if (absolute_time_diff_us(last_interrupt_time, current_time) > 300000)
    {
        if (op == 1)
        {
            clear_display(ssd, frame_area);

            ssd1306_draw_string(ssd, 4, 8, "Joystick-PWM");
            render_on_display(ssd, &frame_area);
            enable_joystick_led_mode = true;
            op += 3;
        }
        else if (op == 2)
        {
            clear_display(ssd, frame_area);

            ssd1306_draw_string(ssd, 4, 8, "Buzzer");
            render_on_display(ssd, &frame_area);
            enable_buzzer_mode = true;
            op += 3;
        }
        else if (op == 3)
        {
            clear_display(ssd, frame_area);

            ssd1306_draw_string(ssd, 4, 8, "Led RGB");
            render_on_display(ssd, &frame_area);
            enable_led_rgb_mode = true;
            op += 3;
        }
        else
        {
            op -= 3;
            back_menu();
        }
        last_interrupt_time = current_time;
    }
}