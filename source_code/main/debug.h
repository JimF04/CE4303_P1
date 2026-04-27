/*
 * debug.h
 *
 *  Created on: Apr 26, 2026
 *      Author: winjimmy
 */

#ifndef MAIN_DEBUG_H_
#define MAIN_DEBUG_H_

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



#endif /* MAIN_DEBUG_H_ */
