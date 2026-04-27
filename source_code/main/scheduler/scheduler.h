#ifndef MAIN_SCHEDULER_SCHEDULER_H_
#define MAIN_SCHEDULER_SCHEDULER_H_

#include "../barcos/barco.h"
#include "../canal/canal.h"
#include "../config.h"

// Tipo de función que devuelve el siguiente barco
typedef barco_t* (*sched_next_fn)(void);

// Cada algoritmo implementa estas funciones
typedef struct {
    void (*init)(canal_t *canal, const config_t *cfg);
    sched_next_fn next;
} scheduler_t;

// Devuelve el scheduler según el config
scheduler_t scheduler_get(const config_t *cfg);

#endif /* MAIN_SCHEDULER_SCHEDULER_H_ */
