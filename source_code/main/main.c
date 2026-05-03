#include <stdio.h>
#include "config.h"
//#include "debug.h"
#include "barcos/barco.h"
#include "canal/canal.h"
#include "canal/flow_policy.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"
#include "driver/uart.h"
#include "string.h"
#include "custom_led.h"


config_t config;
canal_t *canal_global;

scheduler_t sched;
scheduler_t *scheduler_global = &sched;




void app_main(void)
{
    // =========================
	// 1. CONFIG
	// =========================
	if (config_load(&config) < 0) {
	    printf("ERROR cargando config.ini\n");
	    return;
	}
	
	// =========================
	// 2. CANAL
	// =========================
	static canal_t canal;
	
	canal_init(&canal, &config);
	
	canal_global = &canal;
	
	// =========================
	// 3. MUTEX
	// =========================
	
	
	// =========================
	// 4. CREAR BARCOS (TASKS) POR DEFAULT 
	// =========================
	if (config.barcos.cfg_default == 1) {
	    barcos_init(&config);
	}
	
	// =========================
	// 5. SCHEDULER
	// =========================
	sched = scheduler_get(&config);
	
	sched.init(&canal, &config);


    printf("\n===== INICIO (MODELO DISTRIBUIDO) =====\n");

	
	
	uart_init_led();
	
	
    // =========================
    // 5. LOOP PRINCIPAL
    // =========================




    // Crear tarea de input
    xTaskCreate(input_task, "input_task", 4096, NULL, 4, NULL);
	
    while (1) {

    

    // =========================
    // LIMPIAR TERMINADOS
    // =========================
    for (int i = 0; i < barcos_count(); i++) {

        barco_t *b = barcos_get(i); //se toma un barco

        if (!b) continue; //es valido?

        if (b->id != -1 && //esta en el canal
            b->state == DONE && //el estado es done?
            b->pos_canal == -1) {

            printf("[MAIN] Eliminando barco %d (%s)\n",
                   b->id, b->nombre);

            eliminar_barco(i); //se elimina
            i--; //se baja la cantidad de barcos
        }
    }
	
	// =========================
	// TICK DE POLÍTICA (ej: LETRERO)
	// =========================

	if (canal.policy && canal.policy->tick)
	    canal.policy->tick(canal.policy, &canal);

    // =========================
    // SCHEDULER: NUEVO BARCO
    // =========================


    barco_t *nuevo = sched.next(); //el barco que sigue segun el scheduler
	
	if (nuevo != NULL) {
	    if (canal_insertar(&canal, nuevo)) {
	        nuevo->state = RUNNING;
	        if (nuevo->handle != NULL)
	            xTaskNotifyGive(nuevo->handle);
	    } else {
	        printf("[MAIN] WARN: insert falló para barco %d, re-encolando\n", nuevo->id);
	        if (nuevo->id != -1 && nuevo->state != DONE)
	            sched.enqueue(nuevo);
	    }
	}

    // =========================
    // DESPERTAR BARCOS ACTIVOS
    // =========================
    for (int i = 0; i < barcos_count(); i++) {

        barco_t *b = barcos_get(i); //agarrar un barco

        if (!b) continue; //que sea valido

        if (b->pos_canal >= 0) {

            xTaskNotifyGive(b->handle); //se notifica (ahorita no se usa)
        }
    }

    // =========================
    // DEBUG
    // =========================
	led_render_canal(&canal, &sched);
    canal_print(&canal);

    vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

