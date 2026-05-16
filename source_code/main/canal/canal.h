
#ifndef MAIN_CANAL_CANAL_H_
#define MAIN_CANAL_CANAL_H_

#include "../barcos/barco.h"
#include "freertos/semphr.h"

#define MAX_LARGO 50

typedef struct flow_policy flow_policy_t;

typedef struct{
	int largo;
	int direccion_actual; // 0 = izquierda, 1 = derecha
	barco_t *slots[MAX_LARGO]; // posiciones del canal 
	SemaphoreHandle_t slot_mutex[MAX_LARGO]; // un mutex por slot
	SemaphoreHandle_t meta_mutex; // protege solo: ocupacion, direccion_actual
	int ocupacion; // cantidad de barcos dentro del canal
	
	int pasa_buque;

	flow_policy_t *policy;
}canal_t;

typedef struct {
    int largo;
    int direccion_actual;
    int pasa_buque;
    barco_t *slots[MAX_LARGO];  
} canal_snapshot_t;

void canal_snapshot(const canal_t *c, canal_snapshot_t *out);

void canal_init(canal_t *c, const config_t *cfg);

int canal_insertar(canal_t *c, barco_t *b);

void canal_mover_barco(canal_t *c, barco_t *b);

int canal_entrada_segura(canal_t *c, barco_t *nuevo);

int canal_puede_entrar(canal_t *c, barco_t *b);

void canal_print(canal_t *c);

void canal_remover_barco(canal_t *c, barco_t *b);

void canal_viene_buque(canal_t *c);

void buque_task(void *pvParameters);

int canal_lleno(canal_t *c);

barco_t *canal_barco_min(canal_t *c, int criterio);
barco_t *canal_barco_max(canal_t *c, int criterio);



#endif /* MAIN_CANAL_CANAL_H_ */