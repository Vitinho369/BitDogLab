#include "soilHumidity.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int min = 50;
int max = 70;

void configure_soil_humidity()
{
    srand(time(NULL)); // Semente aleatória
}

int read_soil_humidity()
{
    // Simula a leitura de um sensor de umidade do solo, variando de 30 a 70
    return (rand() % (max - min + 1)) + min;
}