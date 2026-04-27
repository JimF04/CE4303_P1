#include <stdio.h>
#include "config.h"
#include "barcos/barco.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

// Metodo para imprimir procesos actuales
void print_tasks_real()
{
    UBaseType_t num = uxTaskGetNumberOfTasks();
    TaskStatus_t *tasks = malloc(num * sizeof(TaskStatus_t));

    uxTaskGetSystemState(tasks, num, NULL);

    printf("\n===== TASK REAL STATE =====\n");

    for (int i = 0; i < num; i++) {

        const char *state;

        switch (tasks[i].eCurrentState) {
            case eRunning:   state = "RUNNING"; break;
            case eReady:     state = "READY"; break;
            case eBlocked:   state = "BLOCKED"; break;
            case eSuspended: state = "SUSPENDED"; break;
            case eDeleted:   state = "DELETED"; break;
            default:         state = "UNKNOWN"; break;
        }

        printf("%-12s | %-10s | Prio: %d\n",
               tasks[i].pcTaskName,
               state,
               tasks[i].uxCurrentPriority);
    }

    free(tasks);
}

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