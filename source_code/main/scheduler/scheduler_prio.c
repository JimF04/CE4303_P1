#include "scheduler.h"
#include "sched_queue.h"
#include <string.h>
#include <stdio.h>

/* ─── colas por dirección ─────────────────────────────── */
static sched_queue_t q_left, q_right;

/* ─── estado del canal ────────────────────────────────── */
static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro
static int orden_global;   // contador global de llegada

extern canal_t *canal_global;

/* ─── enqueue público ─────────────────────────────────── */
static void prio_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, b->prioridad);   // valor = orden de llegada

    printf("[PRIO] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void prio_init(canal_t *canal, const config_t *cfg)
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

        prio_enqueue(b);
    }
}

/* ─── next ───────────────────────────────────────────── */
static barco_t *prio_next(void)
{
	// Un solo barco a la vez
	if (canal_global->ocupacion > 0)
	    return NULL;

    int dir_canal = canal_global->direccion_actual;

    int p_izq = sq_peek_max(&q_left);
    int p_der = sq_peek_max(&q_right);

    if (p_izq == -1 && p_der == -1)
        return NULL;

    barco_t *b = NULL;

    if (dir_canal == 0) {
        b = (p_izq != -1) ? sq_deq_max(&q_left) : NULL;
    } else if (dir_canal == 1) {
        b = (p_der != -1) ? sq_deq_max(&q_right) : NULL;
    } else {
        // Canal libre -> mayor prioridad global; empate -> izquierda
        if (p_izq >= p_der)
            b = sq_deq_max(&q_left);
        else
            b = sq_deq_max(&q_right);
    }

    if (b) {
        b->state = READY;
        printf("[PRIO] -> Barco %d (%s) autorizado (prio=%d, dir=%s)\n",
               b->id, b->nombre, b->prioridad,
               b->direccion == 0 ? "IZQ" : "DER");
    }

    return b;
}

/* ─── notify_done ─────────────────────────────────────── */
static void prio_notify_done(barco_t *b)
{
    if (!b) return;

    b->state = DONE;

    if (en_canal > 0)
        en_canal--;

    printf("[PRIO] Barco %d (%s) salió. en_canal=%d\n",
           b->id, b->nombre, en_canal);
}

/* ─── get_queue (para LEDs) ───────────────────────────── */
static int prio_get_queue(int direccion, barco_t **out, int max)
{
    return sq_get_queue(direccion == 0 ? &q_left : &q_right, out, max);
}

/* ─── release — no-op en PRIO ────────────────────────── */
static void prio_release(void) { }

/* ─── export ──────────────────────────────────────────── */
scheduler_t scheduler_prio = {
    .init        = prio_init,
    .next        = prio_next,
    .release     = prio_release,
    .enqueue     = prio_enqueue,
    .notify_done = prio_notify_done,
    .get_queue   = prio_get_queue,
};