#include "scheduler.h"
#include "sched_queue.h"
#include <string.h>
#include <stdio.h>

/* ─── colas por dirección ─────────────────────────────── */
static sched_queue_t q_left, q_right;

/* ─── estado del canal ────────────────────────────────── */
static int canal_dir;      // dirección activa (-1 = libre)
static int en_canal;       // barcos físicamente dentro
static int orden_global;   // contador global de llegada
extern canal_t *canal_global;

/* ─── enqueue público ─────────────────────────────────── */
static void fcfs_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, orden_global++);   // valor = orden de llegada

    printf("[FCFS] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void fcfs_init(canal_t *canal, const config_t *cfg)
{
    (void)canal;
    (void)cfg;

    sq_init(&q_left);
    sq_init(&q_right);

    orden_global = 0;
    canal_dir    = -1;
    en_canal     = 0;

    // encolar TODOS los barcos
    for (int i = 0; i < barcos_count(); i++) {

        barco_t *b = barcos_get(i);

        if (!b) continue;

        b->en_cola = 0;

        fcfs_enqueue(b);
    }
}

/* ─── next ───────────────────────────────────────────── */
static barco_t *fcfs_next(void)
{
	// Solo permitir si el canal tiene espacio en la entrada
	if (canal_global->ocupacion > 0)
	    return NULL;
	
    int orden_izq = sq_peek_front(&q_left);
    int orden_der = sq_peek_front(&q_right);

    if (orden_izq == 0x7FFFFFFF && orden_der == 0x7FFFFFFF)
        return NULL;

    // Respetar dirección activa del canal
    int dir_canal = canal_global->direccion_actual;

    barco_t *b = NULL;

    if (dir_canal == 0) {
        // Canal ocupado IZQ -> solo sacar de izquierda
        b = (orden_izq != 0x7FFFFFFF) ? sq_deq(&q_left) : NULL;
    } else if (dir_canal == 1) {
        // Canal ocupado DER -> solo sacar de derecha
        b = (orden_der != 0x7FFFFFFF) ? sq_deq(&q_right) : NULL;
    } else {
        // Canal libre -> FCFS global
        b = (orden_izq <= orden_der) ? sq_deq(&q_left) : sq_deq(&q_right);
    }

    if (b) {
        b->state = READY; // main lo pone RUNNING al insertar
        printf("[FCFS] -> Barco %d (%s) autorizado (dir=%s)\n",
               b->id, b->nombre, b->direccion == 0 ? "IZQ" : "DER");
    }

    return b;
}



/* ─── notify_done ─────────────────────────────────────── */
static void fcfs_notify_done(barco_t *b)
{
    if (!b) return; //que el barco sea valido

    b->state = DONE;

    if (en_canal > 0)
        en_canal--;

    printf("[FCFS] Barco %d (%s) salió. en_canal=%d\n",
           b->id, b->nombre, en_canal);
}

/* ─── get_queue (para LEDs) ───────────────────────────── */
static int fcfs_get_queue(int direccion, barco_t **out, int max)
{
    return sq_get_queue(direccion == 0 ? &q_left : &q_right, out, max);
}

/* ─── release — no-op en FCFS ────────────────────────── */
static void fcfs_release(void) { }

/* ─── export ──────────────────────────────────────────── */
scheduler_t scheduler_fcfs = {
    .init        = fcfs_init,
    .next        = fcfs_next,
    .release     = fcfs_release,
    .enqueue     = fcfs_enqueue,
    .notify_done = fcfs_notify_done,
    .get_queue   = fcfs_get_queue,
};