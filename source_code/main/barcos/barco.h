#ifndef MAIN_BARCOS_BARCO_H_
#define MAIN_BARCOS_BARCO_H_

#pragma once
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BARCOS_MAX 8

typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    DONE
} barco_state_t;

typedef struct {
    int id;
    char tipo[16];
    int direccion; // 0 = izquierda, 1 = derecha
    int pos_canal;
    int velocidad;
	char nombre[32];
    TaskHandle_t handle; // referencia al task del barco
	
	barco_state_t state;
	
} barco_t;


void barcos_init(const config_t *cfg);
barco_t* barcos_get(int index);
int barcos_count();
void crear_barco(const config_t *cfg, const char tipo_b[16], int direccion_b);
void asignar_velocidad(const config_t *cfg, barco_t *b);
void eliminar_barco(int index);



#endif /* MAIN_BARCOS_BARCO_H_ */
