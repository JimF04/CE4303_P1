#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "barcos/barco.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"
#include "driver/uart.h"

scheduler_t sched;
config_t config;

void app_main(void)
{
    // Cargar configuración
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



    // 🔥 IMPORTANTE: NO redeclarar sched
    sched = scheduler_get(&config);

    // Inicializar scheduler
    sched.init(&canal, &config);

    // Imprimir procesos
    print_tasks_real();

    static int tick = 0;
    int max_ticks = 50;

    printf("\n\n===== INICIO PROGRAMA =====\n\n");

    // Inicializar UART
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // Crear tarea de input
    xTaskCreate(input_task, "input", 4096, NULL, 5, NULL);

    while (1)
{
    printf("\n========== TICK %d ==========\n", tick++);

    /* 1. Avanzar canal */
    canal_avanzar(&canal);

    /* 2. Detectar barcos terminados y eliminarlos */
    for (int i = 0; i < barcos_count(); i++) {
        barco_t *b = barcos_get(i);
		if (b == NULL) continue;

        if (b != NULL && b->id != -1 && b->state == DONE && b->pos_canal == -1){
            sched.notify_done(b);

            printf("[MAIN] Eliminando barco %d (%s)\n", b->id, b->nombre);

            eliminar_barco(i);
            i--;
        }
    }

    /* 3. Scheduler decide siguiente */
    barco_t *b = sched.next();

    /* 4. Insertar en canal */
    if (b != NULL) {
        printf("[SCHED] Barco %d (%s)\n", b->id, b->nombre);

        if (b->pos_canal < 0 && canal_puede_entrar(&canal, b)) {
            canal_insertar(&canal, b);
        }

        xTaskNotifyGive(b->handle);
    }

    /* 5. Imprimir estado actual */
    canal_print(&canal);

    /* 6. Liberar si aplica */
    if (sched.release) {
        sched.release();
    }

    vTaskDelay(pdMS_TO_TICKS(config.scheduler.quantum_ms));
}
}
