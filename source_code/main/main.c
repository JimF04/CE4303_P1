#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "barcos/barco.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"
#include "driver/uart.h"
#include "string.h"
#include "custom_led.h"


config_t config;
canal_t *canal_global;
SemaphoreHandle_t canal_mutex;

scheduler_t sched;




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
canal_mutex = xSemaphoreCreateMutex();

if (canal_mutex == NULL) {
    printf("ERROR creando mutex\n");
    return;
}

// =========================
// 4. CREAR BARCOS (TASKS)
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

    // =========================
    // 5. LOOP PRINCIPAL
    // =========================




    // Crear tarea de input
    xTaskCreate(input_task, "input", 4096, NULL, 5, NULL);


    while (1) {

    printf("\n========== TICK ==========\n");

    // =========================
    // LIMPIAR TERMINADOS
    // =========================
    for (int i = 0; i < barcos_count(); i++) {

        barco_t *b = barcos_get(i);

        if (!b) continue;

        if (b->id != -1 &&
            b->state == DONE &&
            b->pos_canal == -1) {

            printf("[MAIN] Eliminando barco %d (%s)\n",
                   b->id, b->nombre);

            eliminar_barco(i);
            i--;
        }
    }

    // =========================
    // SCHEDULER: NUEVO BARCO
    // =========================


    barco_t *nuevo = sched.next();

    if (nuevo != NULL) {
            xSemaphoreTake(canal_mutex, portMAX_DELAY);

            canal_insertar(&canal, nuevo);

            xSemaphoreGive(canal_mutex);

            xTaskNotifyGive(nuevo->handle);
        }

    // =========================
    // DESPERTAR BARCOS ACTIVOS
    // =========================
    for (int i = 0; i < barcos_count(); i++) {

        barco_t *b = barcos_get(i);

        if (!b) continue;

        if (b->pos_canal >= 0) {

            xTaskNotifyGive(b->handle);
        }
    }

    // =========================
    // DEBUG
    // =========================
    canal_print(&canal);

    vTaskDelay(pdMS_TO_TICKS(500));
    }
}

