#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "barcos/barco.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"

extern config_t config;
extern scheduler_t sched;
extern canal_t *canal_global;



#define PIN_INPUT GPIO_NUM_11


void input_task(void *arg)
{
    // 🔧 Configurar GPIO 11 como entrada
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_INPUT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    int last_level = 0;

    while (1) {

        int level = gpio_get_level(PIN_INPUT);

        // 🔥 Detectar flanco ascendente (LOW → HIGH)
        if (level == 1 && last_level == 0) {

            char tipo[16];
            int direccion;

            // 👉 Definí aquí qué barco querés crear
            strcpy(tipo, "PAT");
            direccion = 0;

            printf("GPIO HIGH detectado\n");


            
            canal_viene_buque_carepicha(canal_global);





            // if (crear_barco(&config, tipo, direccion)) {

            //     int count = barcos_count();

            //     if (count > 0) {

            //         barco_t *b = barcos_get(count - 1);

            //         if (b != NULL) {
            //             sched.enqueue(b);
            //         }
            //     }
            // }
        }

        last_level = level;

        vTaskDelay(pdMS_TO_TICKS(20)); // pequeño delay para estabilidad
    }
}