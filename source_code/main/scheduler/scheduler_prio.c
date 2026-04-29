#include "scheduler.h"
#include "sched_queue.h"
#include <string.h>
#include <stdio.h>

/* ─── colas por dirección ─────────────────────────────── */
static sched_queue_t q_left, q_right;

/* ─── estado del canal ────────────────────────────────── */
static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro

/* ─── enqueue público ─────────────────────────────────── */
static void prio_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, b->prioridad);   // valor = prioridad

    printf("[PRIO] Barco %d (%s) encolado (prioridad=%d, dir=%s)\n",
           b->id, b->nombre, b->prioridad,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void prio_init(canal_t *canal, const config_t *cfg)
{
    (void)canal; (void)cfg;

    sq_init(&q_left);
    sq_init(&q_right);
    canal_dir = -1;
    en_canal  =  0;

    for (int i = 0; i < barcos_count(); i++)
        prio_enqueue(barcos_get(i));
}

/* ─── next ───────────────────────────────────────────── */
static barco_t *prio_next(void)
{
    // Solo un barco a la vez
    if (en_canal > 0)
        return NULL;

    // Elegir lado con mayor prioridad máxima
    int p_izq = sq_peek_max(&q_left);
    int p_der = sq_peek_max(&q_right);

    // Si ambas colas están vacías
    if (p_izq == -1 && p_der == -1)
        return NULL;

    // Mayor prioridad gana; empate → izquierda
    if (p_izq >= p_der)
        canal_dir = 0;
    else
        canal_dir = 1;

    printf("[PRIO] Canal libre -> dirección elegida: %s (prio izq=%d, der=%d)\n",
           canal_dir == 0 ? "IZQ" : "DER", p_izq, p_der);

    // Extraer el de mayor prioridad del lado elegido
    barco_t *b = (canal_dir == 0)
                 ? sq_deq_max(&q_left)
                 : sq_deq_max(&q_right);

    if (b) {
        b->state = RUNNING;
        en_canal++;
        canal_dir = b->direccion;
        printf("[PRIO] -> Barco %d (%s) autorizado (en_canal=%d)\n",
               b->id, b->nombre, en_canal);
    }

    return b;
}

/* ─── notify_done ─────────────────────────────────────── */
static void prio_notify_done(barco_t *b)
{
    if (!b) return;
    b->state = DONE;
    if (en_canal > 0) en_canal--;
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