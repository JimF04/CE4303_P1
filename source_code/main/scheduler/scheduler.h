#ifndef MAIN_SCHEDULER_SCHEDULER_H_
#define MAIN_SCHEDULER_SCHEDULER_H_

#include "../barcos/barco.h"
#include "../canal/canal.h"
#include "../config.h"

/* =========================
   TIPOS DE FUNCIÓN
   ========================= */
typedef barco_t* (*sched_next_fn)(void);
typedef void (*sched_init_fn)(canal_t *canal, const config_t *cfg);
typedef void (*sched_release_fn)(void);
typedef void (*sched_enqueue_fn)(barco_t *b);   
typedef void (*sched_notify_done_fn)(barco_t *b); 
typedef int (*sched_get_queue_fn)(int direccion, barco_t **out, int max);

/* =========================
   INTERFAZ SCHEDULER
   ========================= */
typedef struct {
    void (*init)(canal_t *canal, const config_t *cfg);
   sched_next_fn next;
	sched_release_fn release;
	sched_enqueue_fn enqueue;       
	sched_notify_done_fn notify_done;
	sched_get_queue_fn get_queue;
}  scheduler_t;

/* =========================
   FACTORY
   ========================= */

scheduler_t scheduler_get(const config_t *cfg);

#endif /* MAIN_SCHEDULER_SCHEDULER_H_ */
