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
#include "events.h"


config_t config;
canal_t *canal_global;

scheduler_t sched;
scheduler_t *scheduler_global = &sched;

QueueHandle_t event_queue;

static void tick_timer_cb(TimerHandle_t xTimer) {
    canal_event_t evt = { .type = EVT_TICK, .data = NULL };
    xQueueSendFromISR(event_queue, &evt, NULL);
}

// La tarea principal:
static void main_task(void *arg) {
    canal_t *canal = (canal_t *)arg;

    canal_event_t evt;
    while (1) {
        // Bloquea aquí — sin consumir CPU — hasta que llegue algo
        if (xQueueReceive(event_queue, &evt, portMAX_DELAY) != pdTRUE)
            continue;

        switch (evt.type) {

			case EVT_BARCO_SALIO: {
			    barco_t *b = (barco_t *)evt.data;
			    for (int i = 0; i < barcos_count(); i++) {
			        if (barcos_get(i) == b) {
			            eliminar_barco(i);
			            break;
			        }
			    }

			    // Aprovechar que el canal se liberó
			    barco_t *nuevo;
			    while ((nuevo = sched.next()) != NULL) {
			        if (canal_insertar(canal, nuevo)) {
			            nuevo->state = RUNNING;
			            if (nuevo->handle)
			                xTaskNotifyGive(nuevo->handle);
			        } else {
			            if (nuevo->id != -1 && nuevo->state != DONE)
			                sched.enqueue(nuevo);
			            break;
			        }
			    }
			    break;
			}

			case EVT_BARCO_LISTO: {
			    // Drenar la queue del scheduler mientras el canal lo permita
			    barco_t *nuevo;
			    while ((nuevo = sched.next()) != NULL) {
			        if (canal_insertar(canal, nuevo)) {
			            nuevo->state = RUNNING;
			            if (nuevo->handle)
			                xTaskNotifyGive(nuevo->handle);
			        } else {
			            // No pudo entrar — re-encolar y parar el intento
			            if (nuevo->id != -1 && nuevo->state != DONE)
			                sched.enqueue(nuevo);
			            break;  // el canal está bloqueado, no tiene sentido seguir
			        }
			    }
			    break;
			}

			case EVT_TICK: {
			    if (canal->policy && canal->policy->tick)
			        canal->policy->tick(canal->policy, canal);

			    // Intentar meter barcos en cada tick también
			    barco_t *nuevo;
			    while ((nuevo = sched.next()) != NULL) {
			        if (canal_insertar(canal, nuevo)) {
			            nuevo->state = RUNNING;
			            if (nuevo->handle)
			                xTaskNotifyGive(nuevo->handle);
			        } else {
			            if (nuevo->id != -1 && nuevo->state != DONE)
			                sched.enqueue(nuevo);
			            break;
			        }
			    }

			    for (int i = 0; i < barcos_count(); i++) {
			        barco_t *b = barcos_get(i);
			        if (b && b->pos_canal >= 0 && b->handle)
			            xTaskNotifyGive(b->handle);
			    }

			    led_render_canal(canal, &sched);
			    canal_print(canal);
				//print_tasks_real();
			    break;
			}

            case EVT_INPUT:
                // input_task ya maneja su lógica
                break;
        }
    }
}

void app_main(void)
{
    // 1. CONFIG
    config_load(&config);

    // 2. CANAL
    static canal_t canal;
    canal_init(&canal, &config);
    canal_global = &canal;

    // 3. QUEUE 
    event_queue = xQueueCreate(20, sizeof(canal_event_t));

    // 4. BARCOS
    if (config.barcos.cfg_default == 1)
        barcos_init(&config);

    // 5. SCHEDULER
    sched = scheduler_get(&config);
    sched.init(&canal, &config);

    // 6. TIMER + TASKS
    uart_init_led();
    TimerHandle_t tick = xTimerCreate("tick", pdMS_TO_TICKS(500),
                                      pdTRUE, NULL, tick_timer_cb);
    xTimerStart(tick, 0);

    xTaskCreate(input_task, "input_task", 4096, NULL, 4, NULL);
    xTaskCreate(main_task,  "main_task",  4096, &canal, 5, NULL);
}

