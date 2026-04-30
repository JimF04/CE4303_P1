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

scheduler_t sched;
config_t config;
canal_t *canal_global;
SemaphoreHandle_t canal_mutex;

void app_main(void)
{
    // =========================
    // 1. CONFIG (si lo usas)
    // =========================
    if (config_load(&config) < 0) {
        printf("ERROR cargando config.ini\n");
        return;
    }

    // =========================
    // 2. CANAL (IMPORTANTE: static)
    // =========================
    static canal_t canal;
    canal_init(&canal, &config);


    canal_global = &canal;

    // =========================
    // 3. MUTEX (CRÍTICO)
    // =========================
    canal_mutex = xSemaphoreCreateMutex();
    if (canal_mutex == NULL) {
        printf("ERROR creando mutex\n");
        return;
    }

    // =========================
    // 4. CREAR BARCOS (tasks)
    // =========================
    if (config.barcos.cfg_default == 1){
        barcos_init(&config);
    }

    printf("\n===== INICIO (MODELO DISTRIBUIDO) =====\n");

    // =========================
    // 5. LOOP PRINCIPAL
    // =========================
    while (1) {

        printf("\n========== TICK ==========\n");

        //LIMPIAR BARCOS TERMINADOS
        for (int i = 0; i < barcos_count(); i++) {
            barco_t *b = barcos_get(i);
            if (!b) continue;

            if (b != NULL && b->id != -1 && b->state == DONE && b->pos_canal == -1) {
                printf("[MAIN] Eliminando barco %d (%s)\n",
                       b->id, b->nombre);

                eliminar_barco(i);
                i--;
            }
        }


        //3. DEBUG / VISUAL
        canal_print(&canal);
        // led_render_canal(&canal, NULL);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

