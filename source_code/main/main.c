#include <stdio.h>
#include "config.h"
#include "barcos/barco.h"

void app_main(void)
{
    config_t config;

    if (config_load(&config) < 0) {
        printf("ERROR cargando config.ini\n");
        return;
    }

    printf("\n=== CONFIG CARGADA ===\n");

    printf("[CANAL]\n");
    printf("largo = %d\n", config.canal.largo);
    printf("metodo_flujo = %s\n", config.canal.metodo_flujo);
    printf("tiempo_letrero_ms = %d\n", config.canal.tiempo_letrero_ms);
    printf("parametro_w = %d\n", config.canal.parametro_w);

    printf("\n[SCHEDULER]\n");
    printf("algoritmo = %s\n", config.scheduler.algoritmo);
    printf("quantum_ms = %d\n", config.scheduler.quantum_ms);
    printf("preemptivo = %d\n", config.scheduler.preemptivo);

    printf("\n[BARCOS]\n");
    printf("cantidad = %d\n", config.barcos.cantidad);
    printf("velocidad_base = %d\n", config.barcos.velocidad_base);
    printf("cfg_default = %d\n", config.barcos.cfg_default);

    printf("izquierda: ");
    for (int i = 0; i < config.barcos.cantidad; i++)
        printf("%s ", config.barcos.izquierda[i]);
    printf("\n");

    printf("derecha: ");
    for (int i = 0; i < config.barcos.cantidad; i++)
        printf("%s ", config.barcos.derecha[i]);
    printf("\n");
	
	
	// Crear barcos por default
	if (config.barcos.cfg_default == 1){
		barcos_init(&config);
	}


	// El scheduler propio va a ir despertando barcos:
	while (1) {
	    barco_t *b = barcos_get(1);

	    // Ejemplo: despierte barco 0
	    xTaskNotifyGive(b->handle);

	    vTaskDelay(pdMS_TO_TICKS(config.scheduler.quantum_ms));
	}
}