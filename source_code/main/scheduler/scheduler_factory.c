#include "scheduler.h"
#include <string.h>

// IMPORTAR ALGORITMOS
extern scheduler_t scheduler_fcfs;

extern scheduler_t scheduler_sjf;


scheduler_t scheduler_get(const config_t *cfg)
{
    if (strcmp(cfg->scheduler.algoritmo, "FCFS") == 0)
        return scheduler_fcfs;

    else if (strcmp(cfg->scheduler.algoritmo, "SJF") == 0)
    {
        return scheduler_sjf;
    }
    

    // default seguro
    return scheduler_fcfs;
}
