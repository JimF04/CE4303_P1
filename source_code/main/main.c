#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "barcos/barco.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"


void app_main(void)
{
	// Cargar configuracion
    config_t config;
    if (config_load(&config) < 0) {
        printf("ERROR cargando config.ini\n");
        return;
    }
	
	// Inicializar canal
	canal_t canal;
	canal_init(&canal, &config);
	
	// Crear barcos por default
	if (config.barcos.cfg_default == 1){
		barcos_init(&config);
	}
	
	// Get el algoritmo pedido por el usuario
	scheduler_t sched = scheduler_get(&config);
	
	// Inicializar algoritmo
	sched.init(&canal, &config);
	
	// Imprimir procesos
	print_tasks_real();
	
	
	static int tick = 0;

	while (1)
	{
	    printf("\n========== TICK %d ==========\n", tick++);

	    barco_t *b = sched.next();

	    printf("[SCHED] Seleccionado barco %d (%s)\n", b->id, b->nombre);

	    if (b->pos_canal < 0 && canal_puede_entrar(&canal, b)) {
	        canal_insertar(&canal, b);
	    }

	    printf("[SCHED] Despertando barco %d\n", b->id);
	    xTaskNotifyGive(b->handle);

	    canal_avanzar(&canal);

	    canal_print(&canal);

	    vTaskDelay(pdMS_TO_TICKS(config.scheduler.quantum_ms));
	}
}