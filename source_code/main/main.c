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

#include "wifi/wifi.h"
#include "wifi/godot.h"

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

	// Wifi
	wifi_init_softap();
	start_ws_server();
	
	// delay para darle chance al UI a conectarse 
	//vTaskDelay(pdMS_TO_TICKS(20000));
	
	// =========================
	// 2. CANAL
	// =========================
	static canal_t canal;
	canal_init(&canal, &config);
	canal_global = &canal;
	
	// =========================
	// 3. SCHEDULER
	// =========================
	sched = scheduler_get(&config);
	sched.init(&canal, &config);
	
	// =========================
	// 4. CREAR BARCOS (TASKS) POR DEFAULT 
	// =========================
	if (config.barcos.cfg_default == 1) {
	    barcos_init(&config);
	}
	
	// Iniciar leds
	//uart_init_led();
	//led_strip_init_custom();


	// Junto a las otras declaraciones estáticas, antes del while:
	static TaskHandle_t led_task_handle = NULL;


	led_strip_init_custom();

	// Crear tarea de input
    xTaskCreate(input_task, "input_task", 4096, NULL, 4, NULL);
	

	// =========================
	// 5. LOOP PRINCIPAL
	// =========================

	printf("\n===== INICIO (MODELO DISTRIBUIDO) =====\n");

    while (1) {

    // =========================
    // LIMPIAR TERMINADOS
    // =========================
	
	for (int i = 0; i < barcos_count(); i++) {
	    barco_t *b = barcos_get(i);
	    if (!b) continue;

	    if (b->id != -1 &&         
	        b->state == DONE && 
	        b->pos_canal == -1) {

	        printf("[MAIN] Eliminando barco %d (%s)\n", b->id, b->nombre);
	        eliminar_barco(i);
	    }
	}
	
	// =========================
	// TICK DE POLÍTICA 
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
				printf("[MAIN] Barco %d insertado, se mueve en el siguiente tick\n", nuevo->id);
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

        if (!b) {
			continue; //que sea valido
		} 
		
		if (b->id == -1) continue; 
		
		if (b->pos_canal >= 0 && b->handle != NULL) {
		    xTaskNotifyGive(b->handle);
		}
    }

    // =========================
    // DEBUG
    // =========================
	canal_print(&canal);
	godot_export_state(&canal, &sched);
	led_render_canal(&canal, &sched);
	
    vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

