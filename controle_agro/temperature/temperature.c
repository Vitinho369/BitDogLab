#include "temperature.h"
#include "hardware/adc.h"

float temperature = 0;

void configure_temp_sensor() // Configura o sensor de temperatura
{
    adc_set_temp_sensor_enabled(true); // Habilita o sensor de temperatura interno
}

float read_temperature()
{
    adc_select_input(4);
    uint16_t raw = adc_read();

    // Converte o valor lido para voltagem (0-3.3V)
    float voltage = raw * 3.3f / (1 << 12);

    // Converte a voltagem para temperatura
    temperature = 27 - (voltage - 0.706) / 0.001721;
    return temperature;
}