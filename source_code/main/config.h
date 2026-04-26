#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int largo;
    char metodo_flujo[16];
    int tiempo_letrero_ms;
    int parametro_w;
} canal_cfg_t;

typedef struct {
    char algoritmo[16];
    int quantum_ms;
    int preemptivo;
} scheduler_cfg_t;

typedef struct {
    int cantidad;
    int velocidad_base;
    int cfg_default;
    char izquierda[10][16];
    char derecha[10][16];
} barcos_cfg_t;

typedef struct {
    canal_cfg_t canal;
    scheduler_cfg_t scheduler;
    barcos_cfg_t barcos;
} config_t;

// API pública
int config_load(config_t* cfg);


#endif /* MAIN_CONFIG_H_ */
