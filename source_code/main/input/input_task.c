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
#define PIN_BAR  GPIO_NUM_10
#define PIN_BAR2 GPIO_NUM_8

// Tiempo máximo entre pulsos para agruparlos
#define MULTI_PRESS_WINDOW_MS 800


void crear_barco_por_pulsos(int pulsos, int direccion)
{
    char tipo[16];

    if (pulsos >= 3) {
        strcpy(tipo, "PAT");
    }
    else if (pulsos == 2) {
        strcpy(tipo, "PES");
    }
    else {
        strcpy(tipo, "NOR");
    }

    printf("Creando barco tipo: %s, direccion: %d\n",
           tipo, direccion);

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


void input_task(void *arg)
{
    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << PIN_INTE) |
            (1ULL << PIN_BAR)  |
            (1ULL << PIN_BAR2),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    int last_level_int  = 0;
    int last_level_bar  = 0;
    int last_level_bar2 = 0;

    //-----------------------------------------
    // Contador para PIN_BAR (direccion 0)
    //-----------------------------------------
    int bar_press_count = 0;
    TickType_t first_press_time = 0;

    //-----------------------------------------
    // Contador para PIN_BAR2 (direccion 1)
    //-----------------------------------------
    int bar2_press_count = 0;
    TickType_t first_press_time2 = 0;

    while (1) {
        int level_int  = gpio_get_level(PIN_INTE);
        int level_bar  = gpio_get_level(PIN_BAR);
        int level_bar2 = gpio_get_level(PIN_BAR2);

        TickType_t now = xTaskGetTickCount();

        //---------------------------------------------------
        // Detectar flanco ascendente botón BAR (direccion 0)
        //---------------------------------------------------
        if (level_bar == 1 && last_level_bar == 0) {

            if (bar_press_count == 0) {
                first_press_time = now;
            }

            bar_press_count++;

            printf("Pulso BAR detectado (%d)\n",
                   bar_press_count);
        }

        //---------------------------------------------------
        // Detectar flanco ascendente botón BAR2 (direccion 1)
        //---------------------------------------------------
        if (level_bar2 == 1 && last_level_bar2 == 0) {

            if (bar2_press_count == 0) {
                first_press_time2 = now;
            }

            bar2_press_count++;

            printf("Pulso BAR2 detectado (%d)\n",
                   bar2_press_count);
        }

        //---------------------------------------------------
        // Resolver BAR (direccion 0)
        //---------------------------------------------------
        if (bar_press_count > 0 &&
            (now - first_press_time) >=
            pdMS_TO_TICKS(MULTI_PRESS_WINDOW_MS))
        {
            crear_barco_por_pulsos(
                bar_press_count,
                0
            );

            bar_press_count = 0;
        }

        //---------------------------------------------------
        // Resolver BAR2 (direccion 1)
        //---------------------------------------------------
        if (bar2_press_count > 0 &&
            (now - first_press_time2) >=
            pdMS_TO_TICKS(MULTI_PRESS_WINDOW_MS))
        {
            crear_barco_por_pulsos(
                bar2_press_count,
                1
            );

            bar2_press_count = 0;
        }

        //---------------------------------------------------
        // Botón de buque intacto
        //---------------------------------------------------
        if (level_int == 1 &&
            last_level_int == 0)
        {
            printf(
                "ay viene el buque que miedo!!!!\n"
            );

            canal_viene_buque(
                canal_global
            );
        }

        last_level_int  = level_int;
        last_level_bar  = level_bar;
        last_level_bar2 = level_bar2;

        vTaskDelay(
            pdMS_TO_TICKS(20)
        );
    }
}
