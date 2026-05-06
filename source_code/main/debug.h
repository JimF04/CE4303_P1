#ifndef MAIN_DEBUG_H_
#define MAIN_DEBUG_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <stdio.h>

void print_tasks_real()
{
    // Suspender el scheduler para lectura consistente
    vTaskSuspendAll();

    // Pedir cuántas tareas hay AHORA
    UBaseType_t num = uxTaskGetNumberOfTasks();

    // Crear buffer seguro
    TaskStatus_t *tasks = malloc(num * sizeof(TaskStatus_t));
    if (!tasks) {
        xTaskResumeAll();
        printf("ERROR: malloc\n");
        return;
    }

    // Obtener el estado real de TODAS las tareas
    UBaseType_t realNum = uxTaskGetSystemState(tasks, num, NULL);

    // Reanudar scheduler
    xTaskResumeAll();

    printf("\n===== TASK REAL STATE =====\n");

    for (int i = 0; i < realNum; i++) {

        const char *state;

        switch (tasks[i].eCurrentState) {
            case eRunning:   state = "RUNNING"; break;
            case eReady:     state = "READY"; break;
            case eBlocked:   state = "BLOCKED"; break;
            case eSuspended: state = "SUSPENDED"; break;
            case eDeleted:   state = "DELETED"; break;
            default:         state = "UNKNOWN"; break;
        }

		printf("%-12s | %-10s | Prio: %2u | StackFree: %4lu bytes\n",
		       tasks[i].pcTaskName,
		       state,
		       (unsigned)tasks[i].uxCurrentPriority,
		       (unsigned long)(tasks[i].usStackHighWaterMark * sizeof(StackType_t)));
    }

    free(tasks);
}

#endif /* MAIN_DEBUG_H_ */