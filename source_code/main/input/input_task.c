#include "driver/uart.h"
#include "barcos/barco.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"

extern config_t config;
extern scheduler_t sched;

#define INPUT_UART UART_NUM_1

void uart_init_input(void)
{
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(INPUT_UART, 2048, 0, 0, NULL, 0);
    uart_param_config(INPUT_UART, &cfg);

    uart_set_pin(INPUT_UART,
        5,   // TX (opcional)
        4,   // RX (teclado/input)
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );
}

void input_task(void *arg)
{
    char c;

    while (1) {
        int len = uart_read_bytes(INPUT_UART, &c, 1, pdMS_TO_TICKS(100));

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