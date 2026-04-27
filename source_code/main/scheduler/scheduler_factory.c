#include "scheduler.h"
#include <string.h>

// IMPORTAR ALGORITMOS
extern scheduler_t scheduler_fcfs;


scheduler_t scheduler_get(const config_t *cfg)
{
    if (strcmp(cfg->scheduler.algoritmo, "FCFS") == 0)
        return scheduler_fcfs;

    // default seguro
    return scheduler_fcfs;
}