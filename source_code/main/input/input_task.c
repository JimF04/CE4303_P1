#include "driver/uart.h"
#include "barcos/barco.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"

extern config_t config;
extern scheduler_t sched;

void input_task(void *arg)
{
    char c;

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, &c, 1, pdMS_TO_TICKS(100));

        if (len > 0) {

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
}