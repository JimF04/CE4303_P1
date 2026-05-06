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



#define PIN_INTE GPIO_NUM_11
#define PIN_BAR GPIO_NUM_10


void input_task(void *arg)
{
    //Configurar GPIO 11 como entrada
    gpio_config_t io_conf1 = {
        .pin_bit_mask = (1ULL << PIN_INTE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << PIN_BAR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };




    gpio_config(&io_conf1);

    gpio_config(&io_conf2);

    int last_level_int = 0;

    int last_level_bar = 0;

    while (1) {

        int level_int = gpio_get_level(PIN_INTE);
        int level_bar = gpio_get_level(PIN_BAR);


        //Detectar flanco ascendente (LOW → HIGH)
        if (level_bar == 1 && last_level_bar == 0) {

            char tipo[16];
            int direccion;

            // Definí aquí qué barco querés crear
            strcpy(tipo, "PAT");
            direccion = 0;

            printf("Otro sapo, que no haya segmentation porfa\n");

            if (crear_barco(&config, tipo, direccion)) {

                int count = barcos_count();

                if (count > 0) {

                    barco_t *b = barcos_get(count - 1);

                    if (b != NULL) {
                        sched.enqueue(b);
                    }
                }
            }
        }

        if (level_int == 1 && last_level_int == 0) {

            printf("ay viene el buque que miedo!!!!\n");
            canal_viene_buque(canal_global);

        }


        last_level_int = level_int;
        last_level_bar = level_bar;

        vTaskDelay(pdMS_TO_TICKS(20)); // pequeño delay para estabilidad
    }
}