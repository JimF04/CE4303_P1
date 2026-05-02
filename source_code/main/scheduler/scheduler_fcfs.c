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
    if (b->id == -1) return;           // slot liberado
    if (b->pos_canal >= 0) return;     // ya está dentro del canal
    if (b->state == DONE) return;      // ya terminó

    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, orden_global++);

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
    if (canal_global->ocupacion > 0)
        return NULL;

    int orden_izq = sq_peek_front(&q_left);
    int orden_der = sq_peek_front(&q_right);

    if (orden_izq == 0x7FFFFFFF && orden_der == 0x7FFFFFFF)
        return NULL;

    int dir_canal = canal_global->direccion_actual;

    // Construir orden de preferencia de direcciones
    int primera, segunda;

    if (dir_canal == 0) {
        primera = 0; segunda = -1;  // solo IZQ
    } else if (dir_canal == 1) {
        primera = 1; segunda = -1;  // solo DER
    } else {
        // Canal libre: preferir por orden FCFS, pero probar ambas
        if (orden_izq <= orden_der) {
            primera = 0; segunda = 1;
        } else {
            primera = 1; segunda = 0;
        }
    }

    // Intentar primera dirección
    int dirs[2] = { primera, segunda };
    for (int i = 0; i < 2; i++) {
        int d = dirs[i];
        if (d == -1) break;

        sched_queue_t *q = (d == 0) ? &q_left : &q_right;
        int orden = (d == 0) ? orden_izq : orden_der;
        if (orden == 0x7FFFFFFF) continue;

        barco_t *candidato = sq_peek_barco(q);
        if (!candidato || candidato->id == -1 || candidato->state == DONE) {
            sq_deq(q);  // limpiar entrada inválida
            continue;
        }

        if (!canal_puede_entrar(canal_global, candidato))
            continue;  // esta dirección bloqueada por política, probar la otra

        // Aceptado
        barco_t *b = sq_deq(q);
        b->state = READY;
        printf("[FCFS] -> Barco %d (%s) autorizado (dir=%s)\n",
               b->id, b->nombre, b->direccion == 0 ? "IZQ" : "DER");
        return b;
    }

    return NULL;  // ninguna dirección disponible este tick
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