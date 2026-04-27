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

	int max_ticks = 46;

	while (tick < max_ticks)
	{
	    printf("\n========== TICK %d ==========\n", tick++);

	    /* 1. Avanzar canal PRIMERO */
	    canal_avanzar(&canal);

	    /* 2. Detectar salidas */
	    for (int i = 0; i < barcos_count(); i++) {
	        barco_t *b = barcos_get(i);
	        if (b->state == DONE && b->pos_canal == -1) {
	            sched.notify_done(b);
	            b->pos_canal = -2;
	        }
	    }

	    /* 3. Scheduler decide siguiente */
	    barco_t *b = sched.next();

	    /* 4. Insertar en canal */
	    if (b) {
	        printf("[SCHED] Barco %d (%s)\n", b->id, b->nombre);
	        if (b->pos_canal < 0 && canal_puede_entrar(&canal, b))
	            canal_insertar(&canal, b);
	        xTaskNotifyGive(b->handle);
	    }

	    /* 5. Imprimir estado actual */
	    canal_print(&canal);
		
		print_tasks_real();

	    if (sched.release)
	        sched.release();

			

	    vTaskDelay(pdMS_TO_TICKS(config.scheduler.quantum_ms));
	}
	

}