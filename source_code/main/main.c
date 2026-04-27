#include <stdio.h>
#include "config.h"
#include "barcos/barco.h"
#include "debug.h"

void app_main(void)
{
	// Cargar configuracion
    config_t config;
    if (config_load(&config) < 0) {
        printf("ERROR cargando config.ini\n");
        return;
    }
	
	// Crear barcos por default
	if (config.barcos.cfg_default == 1){
		barcos_init(&config);
	}
	
	// Imprimir procesos
	print_tasks_real();


	// El scheduler propio va a ir despertando barcos:
//	while (1) {
//	    barco_t *b = barcos_get(1);
//
//	    // Ejemplo: despierte barco 0
//	    xTaskNotifyGive(b->handle);
//
//	    vTaskDelay(pdMS_TO_TICKS(config.scheduler.quantum_ms));
//	}
}