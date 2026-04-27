#ifndef MAIN_BARCOS_BARCO_H_
#define MAIN_BARCOS_BARCO_H_

#pragma once
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    int id;
    char tipo[16];
    int direccion; // 0 = izquierda → derecha, 1 = derecha → izquierda
    int posicion;
    int velocidad;
    TaskHandle_t handle; // referencia al task del barco
} barco_t;


void barcos_init(const config_t *cfg);
barco_t* barcos_get(int index);
int barcos_count();
void crear_barco(const config_t *cfg, const char tipo_b[16], int direccion_b);
void asignar_velocidad(const config_t *cfg, barco_t *b);



#endif /* MAIN_BARCOS_BARCO_H_ */
