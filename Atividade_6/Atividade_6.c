/*O sinal amarelo foi retirado, pois em um semáforo de pedestres não existe este sinal, dessa forma
foi mantida sua mensagem, ao apertar o botão para indicar aos pedestres que em breve o sinal será aberto,
seguindo o funcionamento normal de um semáforo de pedestres.*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

#define BTN_A_PIN 5

int A_state = 0;

char *textAberto[] = {
    " SINAL ABERTO ",
    "ATRAVESSAR COM ",
    "    CUIDADO "};

char *textAtencao[] = {
    "   SINAL DE ",
    "    ATENCAO ",
    "   PREPARE-SE "};

char *textFechado[] = {
    "     SINAL ",
    "    FECHADO ",
    "    AGUARDE"};

void limpaDisplay(uint8_t ssd[], struct render_area frame_area)
{
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void SinalAberto(uint8_t ssd[], struct render_area frame_area)
{
    limpaDisplay(ssd, frame_area);
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);

    int y = 0;
    for (uint i = 0; i < count_of(textAberto); i++)
    {
        ssd1306_draw_string(ssd, 5, y, textAberto[i]);
        y += 16;
    }
    render_on_display(ssd, &frame_area);
}

void SinalAtencao(uint8_t ssd[], struct render_area frame_area)
{
    limpaDisplay(ssd, frame_area);

    int y = 0;
    for (uint i = 0; i < count_of(textAtencao); i++)
    {
        ssd1306_draw_string(ssd, 5, y, textAtencao[i]);
        y += 16;
    }
    render_on_display(ssd, &frame_area);
}

void SinalFechado(uint8_t ssd[], struct render_area frame_area)
{
    limpaDisplay(ssd, frame_area);
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);

    int y = 0;
    for (uint i = 0; i < count_of(textFechado); i++)
    {
        ssd1306_draw_string(ssd, 5, y, textFechado[i]);
        y += 16;
    }
    render_on_display(ssd, &frame_area);
}

int WaitWithRead(int timeMS)
{
    for (int i = 0; i < timeMS; i = i + 100)
    {
        A_state = !gpio_get(BTN_A_PIN);
        if (A_state == 1)
        {
            return 1;
        }
        sleep_ms(100);
    }
    return 0;
}

int main()
{
    stdio_init_all();

    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

    while (true)
    {
        SinalFechado(ssd, frame_area);
        A_state = WaitWithRead(13000);

        if (A_state)
        {
            SinalAtencao(ssd, frame_area);
            sleep_ms(5000);

            SinalAberto(ssd, frame_area);
            sleep_ms(8000);
        }
        else
        {
            SinalAberto(ssd, frame_area);
            sleep_ms(8000);
        }
    }

    return 0;
}
