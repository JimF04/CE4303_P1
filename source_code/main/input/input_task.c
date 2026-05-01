#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "barcos/barco.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"

extern config_t config;
extern scheduler_t sched;

void input_task(void *arg)
{
    // 🔥 hacer stdin no bloqueante
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    while (1) {

        int c = getchar();

        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        char tipo[16];
        int direccion;

        if (c == 'q') {
            strcpy(tipo, "PAT");
            direccion = 0;
        }
        else if (c == 'w') {
            strcpy(tipo, "PES");
            direccion = 1;
        }
        else if (c == 'e') {
            strcpy(tipo, "NOR");
            direccion = 0;
        }
        else {
            continue;
        }

        printf("Input: %c\n", (char)c);

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
}