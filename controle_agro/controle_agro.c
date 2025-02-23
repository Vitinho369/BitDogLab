#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "client/client.h"
#include "display/display.h"
#include "temperature/temperature.h"
#include "button/button.h"
#include "joystick/joystick.h"
#include "motor/motor.h"
#include "soilHumidity/soilHumidity.h"

// Variáveis para controle do menu
int op = 1;
const int sw_pin = 22;
static absolute_time_t last_interrupt_time = 0;   // Para debounce do botão do joystick
static absolute_time_t last_interrupt_time_A = 0; // Para debounce do botão do botão A

// Botões para controle de motores
const int buttonA = 5;

// Variáveis e constantes para envio de dados
struct tcp_pcb *pcb;
char request[200];
const char *text_request = "GET /update.json?api_key=Z9F11EMPXLGYJB7G&field1=%.2f&field2=%d HTTP/1.1\r\n"
                           "Host: api.thingspeak.com\r\n"
                           "Connection: close\r\n\r\n";

// Variável para envio de dados ao ThingSpeak
ip_addr_t server_ip;
volatile bool send = false;
const char *ssid = "NOME DA REDE";
const char *password = "SENHA DA REDE";

struct repeating_timer timer;

// variáveis para controle de modos
bool temperature_screen_mode = false;
bool water_bomb_screen_mode = false;
bool soil_humidity_screen_mode = false;

int soil_humidity = 0;

// Assinatura das funções de interrupçãos
bool repeating_timer_callback(struct repeating_timer *t);
void gpio_callback(uint gpio, uint32_t events);
bool on_motor = 0;

void send_data();
int main()
{
    // Inicializando todos os periféricos utilizados
    stdio_init_all();
    configure_joystick();
    configure_motor();
    configure_buttons();
    configure_soil_humidity();
    configure_display();
    configure_temp_sensor();

    int y0 = 5, y1 = 18;
    init_system_message(); // Apresenta mensagem de inicialização antes da conexão wifi ser bem sucedida

    // Definindo interrupção para o sistema
    add_repeating_timer_ms(20000, repeating_timer_callback, NULL, &timer);
    gpio_set_irq_enabled_with_callback(sw_pin, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(buttonA, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    while (!connect_wifi(ssid, password)) // Tenta conectar ao wifi
    {
        printf("Tentando novamente...\n");
    }

    resolve_name("api.thingspeak.com", &server_ip);

    printf("IP resolvido: %s\n", ipaddr_ntoa(&server_ip));

    soil_humidity = read_soil_humidity(); // Lê a umidade do solo para apresentar no display
    draw_menu(y0, y1);                    // Desenha o menu principal quando a conexão wifi é bem sucedida

    while (true)
    {
        if (temperature_screen_mode)
        {
            temperature_screen(read_temperature()); // Apresenta a temperatura no display
        }
        else if (water_bomb_screen_mode)
        {
            water_bomb_screen(on_motor); // Apresenta o estado da bomba d'agua no display
        }
        else if (soil_humidity_screen_mode)
        {
            soil_humidity_screen(soil_humidity); // Apresenta a umidade do solo no display
        }
        else
        {
            move_line_select(&y0, &y1, &op); // Permite o usuário interagir com as opções do menu
        }

        if (send) // Quando o temporizador ativa, envia os dados ao ThingSpeak
        {
            send_data();
            send = false; // Reseta a flag de envio
        }

        sleep_ms(100);
    }
    return 0;
}

void send_data() // Função para enviar dados ao ThingSpeak
{
    soil_humidity = read_soil_humidity();
    snprintf(request, sizeof(request), text_request, read_temperature(), soil_humidity);

    pcb = tcp_new();
    client_create(pcb, &server_ip, 80);
    client_write(pcb, request);
    sleep_ms(150);
    client_close(pcb);
}

// Implementação das funções de interrupção
bool repeating_timer_callback(struct repeating_timer *t) // Habilita o envio de dados a cada 15 segundos
{
    send = true;
    return true;
}

void gpio_callback(uint gpio, uint32_t events) // Função para tratar interrupções
{
    absolute_time_t current_time = get_absolute_time();

    if (gpio == sw_pin) // Verifica se foi o botão do joystick apertado
    {
        if (absolute_time_diff_us(last_interrupt_time, current_time) > 300000) // Debounce
        {
            if (op == 1) // Habilita a tela de temperatura
            {
                temperature_screen_mode = true;
                op += 3;
            }
            else if (op == 2) // Habilita a tela da bomba d'agua
            {
                water_bomb_screen_mode = true;
                op += 3;
            }
            else if (op == 3) // Habilita a tela de umidade do solo
            {
                soil_humidity_screen_mode = true;
                op += 3;
            }
            else // Volta para o menu principal
            {
                // Desabilita todas as telas e volta para apresentar o menu principal
                temperature_screen_mode = false;
                water_bomb_screen_mode = false;
                soil_humidity_screen_mode = false;
                op -= 3;
                back_menu(op);
            }
            last_interrupt_time = current_time;
        }
    }
    else if (water_bomb_screen_mode && gpio == buttonA) // Verifica se o botão A foi apertado quando a tela da bomba estiver aberta
    {
        if (absolute_time_diff_us(last_interrupt_time_A, current_time) > 500000) // Debounce
        {
            on_motor = 1 ^ on_motor; // Liga e desliga o sinal enviado para o motor, usando a função lógica XOR
            set_motor(on_motor);
            last_interrupt_time_A = current_time;
        }
    }
}