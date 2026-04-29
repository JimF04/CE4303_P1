#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void uart_init_input();

// Prototipo de la tarea
void input_task(void *arg);

#endif