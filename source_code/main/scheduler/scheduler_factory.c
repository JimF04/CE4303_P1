#include "scheduler.h"
#include <string.h>

// IMPORTAR ALGORITMOS
extern scheduler_t scheduler_fcfs;

extern scheduler_t scheduler_sjf;

extern scheduler_t scheduler_prio;

extern scheduler_t scheduler_strn;


extern scheduler_t scheduler_rr;



scheduler_t scheduler_get(const config_t *cfg)
{
    if (strcmp(cfg->scheduler.algoritmo, "FCFS") == 0)
        return scheduler_fcfs;

    else if (strcmp(cfg->scheduler.algoritmo, "SJF") == 0)
    {
        return scheduler_sjf;
    }
    else if (strcmp(cfg->scheduler.algoritmo, "PRIO") == 0)
    {
        return scheduler_prio;
    }
    else if (strcmp(cfg->scheduler.algoritmo, "STRN") == 0)
    {
        return scheduler_strn;
    }
    else if (strcmp(cfg->scheduler.algoritmo, "RR") == 0)
    {
        return scheduler_rr;
    }

    // default seguro
    return scheduler_fcfs;
}
