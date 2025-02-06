#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/clocks.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

//Definição dos pinos utilizados no projeto
const uint LED_VERMELHO = 6;
const uint LED_AMARELO = 7;
const uint LED_VERDE = 8;
const uint LED_PEDESTRE = 3;
const uint BUZZER = 28;
const uint BOTAO = 19;

// Frequência do buzzer (em Hz)
const uint BUZZER_FREQUENCY = 100;

void ascender_verde(){
  gpio_put(LED_VERMELHO, false);
  gpio_put(LED_VERDE, true);
  gpio_put(LED_AMARELO, false);
  gpio_put(LED_PEDESTRE, false);
}

void ascender_amarelo(){ 
  gpio_put(LED_VERMELHO, false);
  gpio_put(LED_VERDE, false);
  gpio_put(LED_AMARELO, true);
  gpio_put(LED_PEDESTRE, false);
}

void ascender_vermelho(){
  gpio_put(LED_VERMELHO, true);
  gpio_put(LED_VERDE, false);
  gpio_put(LED_AMARELO, false);
  gpio_put(LED_PEDESTRE, false);
}

void pedestre_atravessando(){ 
  gpio_put(LED_VERMELHO, true);
  gpio_put(LED_VERDE, false);
  gpio_put(LED_AMARELO, false);
  gpio_put(LED_PEDESTRE, true);
}

// Definição de uma função para inicializar o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0);
}

void beep(uint pin, uint32_t tempo_atual, uint32_t *tempo_toque, bool *toque) {
    
    uint slice_num = pwm_gpio_to_slice_num(pin);// Obter o slice do PWM associado ao pino

    if((tempo_atual - *tempo_toque) < 500 && *toque){ //Aciona o buzzer em um intervalo de 500 ms
      
      pwm_set_gpio_level(pin, 2048);

    }else if((tempo_atual - *tempo_toque) < 500 && !*toque){ //Desativa o buzzer em um intervalo de 500 ms
    
      pwm_set_gpio_level(pin, 0);  

    }else if(*toque){//Reinicia tempo para que se possa contar novamente o intervalo de 500 ms
     
      *toque = false; 
      *tempo_toque = to_ms_since_boot(get_absolute_time());
    }else{//Reinicia tempo para que se possa contar novamente o intervalo de 500 ms
      
      *toque = true; //Indica que o buzzer deve emitir som
      *tempo_toque = to_ms_since_boot(get_absolute_time());
    }
}

void desativa_beep(uint pin){

  uint slice_num = pwm_gpio_to_slice_num(pin); // Obter o slice do PWM associado ao pino
  pwm_set_gpio_level(pin, 0); 
}

int main() {
  //Inicia pinos do Raspberry pi Pico
  stdio_init_all(); 
  gpio_init(LED_VERMELHO);
  gpio_init(LED_VERDE);
  gpio_init(LED_AMARELO);
  gpio_init(LED_PEDESTRE);
  gpio_init(BOTAO);
  gpio_init(BUZZER);

  pwm_init_buzzer(BUZZER); //Inicializa o buzzer
  
  //Configura os pinos de entrada e saída
  gpio_set_dir(LED_VERMELHO, GPIO_OUT);
  gpio_set_dir(LED_VERDE, GPIO_OUT);
  gpio_set_dir(LED_AMARELO, GPIO_OUT);
  gpio_set_dir(LED_PEDESTRE, GPIO_OUT);
  gpio_set_dir(BUZZER, GPIO_OUT);
  gpio_set_dir(BOTAO, GPIO_IN);
  gpio_pull_up(BOTAO); //Define o botão como pull-up

  //Variáveis para controle do intervalo de tempo dos leds
  uint32_t tempo_verde = to_ms_since_boot(get_absolute_time());
  uint32_t tempo_amarelo = 0;
  uint32_t tempo_vermelho = 0;
  uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

  uint32_t tempo_toque = 0; // Variável para intervalo de ativação e desativação do buzzer
  bool toque = false; // Variável para definir o modo de ativação do buzzer (ligado/desligado)
  bool pedestre_modo = false; // Variável que define o modo de funcionamento do semáforo (carro/pedestres)

  while (true) {
    bool estado_botao = gpio_get(BOTAO);

    if(!estado_botao){ //Verifica se o botão foi pressionado

      if(!pedestre_modo){ // Se não exister no modo de travessia do pedestre, ative o modo de travessia e inicie o tempo do sinal amarelo
        
        tempo_amarelo = to_ms_since_boot(get_absolute_time()); // Pega o tempo que se passou desde que o Raspberry Pi Pico foi ligado
        pedestre_modo = true; 
      }

    }
    
    if(!pedestre_modo){ // Se o modo de travessia não estiver habilitado, o sinal deve funcionar normalmente

      if(tempo_atual - tempo_verde < 8000){ 
       
        ascender_verde();
        tempo_amarelo = to_ms_since_boot(get_absolute_time());

      }else if(tempo_atual - tempo_amarelo < 2000){ 
 
        ascender_amarelo();
        tempo_vermelho = to_ms_since_boot(get_absolute_time());

      }else if(tempo_atual - tempo_vermelho < 10000){
        ascender_vermelho(); 

      }else{ // Caso chegue aqui, será o sinal será reiniciado e voltará ao estado sinal e ligará o led verde
       
        tempo_verde = to_ms_since_boot(get_absolute_time());
      
      }

    }else{ // Se o modo de passagem de pedestres estiver ativado

      if(tempo_atual - tempo_amarelo < 5000){
        ascender_amarelo();
        tempo_vermelho = to_ms_since_boot(get_absolute_time());

      }else if(tempo_atual - tempo_vermelho < 15000){
        pedestre_atravessando();
        beep(BUZZER, tempo_atual, &tempo_toque, &toque);
      }else{
        pedestre_modo = false;
        desativa_beep(BUZZER);
      }

    }

    sleep_ms(10); // Para funcionamento correto do wokwi
    tempo_atual = to_ms_since_boot(get_absolute_time());
  }
}