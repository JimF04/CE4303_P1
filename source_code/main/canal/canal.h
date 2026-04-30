
#ifndef MAIN_CANAL_CANAL_H_
#define MAIN_CANAL_CANAL_H_

#include "../barcos/barco.h"

#define MAX_LARGO 50

typedef struct flow_policy flow_policy_t;

typedef struct{
	int largo;
	int direccion_actual; // 0 = izquierda, 1 = derecha
	
	// posiciones del canal (cada celda apunta a un barco o NULL)
	barco_t *slots[MAX_LARGO]; 
	
	// cantidad de barcos dentro del canal
	int ocupacion;
	
	int barcos_pasados;
	int limite_w;
	
	flow_policy_t *policy;
}canal_t;

void canal_init(canal_t *c, const config_t *cfg);

void canal_insertar(canal_t *c, barco_t *b);

void canal_mover_barco(canal_t *c, barco_t *b);

int canal_puede_entrar(canal_t *c, barco_t *b);

void canal_print(canal_t *c);

#endif /* MAIN_CANAL_CANAL_H_ */
