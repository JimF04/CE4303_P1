#ifndef MAIN_EVENTS_H_
#define MAIN_EVENTS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    EVT_BARCO_SALIO,
    EVT_BARCO_LISTO,
    EVT_TICK,
    EVT_INPUT,
} event_type_t;

typedef struct {
    event_type_t type;
    void *data;
} canal_event_t;

extern QueueHandle_t event_queue;

#endif /* MAIN_EVENTS_H_ */
