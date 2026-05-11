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
static void sjf_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, b->velocidad);   // valor = orden de llegada

    printf("[SJF] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void sjf_init(canal_t *canal, const config_t *cfg)
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

        sjf_enqueue(b);
    }
}


/* ─── next ───────────────────────────────────────────── */
static barco_t *sjf_next(void)
{
    // Un solo barco a la vez
//    if (canal_global->ocupacion > 0)
//        return NULL;

    int dir_canal = canal_global->direccion_actual;

    int v_izq = sq_peek_max(&q_left);
    int v_der = sq_peek_max(&q_right);

    if (v_izq == -1 && v_der == -1)
        return NULL;

    // Orden de preferencia según dirección del canal
    int primera, segunda;
    if (dir_canal == 0) {
        primera = 0; segunda = -1;  // solo IZQ
    } else if (dir_canal == 1) {
        primera = 1; segunda = -1;  // solo DER
    } else {
        // Canal libre -> el más rápido global; empate -> izquierda
        if (v_izq >= v_der) { primera = 0; segunda = 1; }
        else                 { primera = 1; segunda = 0; }
    }

    // Intentar en orden de preferencia
    int dirs[2] = { primera, segunda };
    for (int i = 0; i < 2; i++) {
        int d = dirs[i];
        if (d == -1) break;

        sched_queue_t *q = (d == 0) ? &q_left : &q_right;
        int vmax = (d == 0) ? v_izq : v_der;
        if (vmax == -1) continue;

        // Peek sin desencolar todavía
        barco_t *candidato = sq_peek_barco_max(q);
        if (!candidato || candidato->id == -1 || candidato->state == DONE) {
            sq_deq_max(q);  // limpiar entrada inválida
            continue;
        }

        // Consultar al canal si puede entrar ANTES de desencolar
        if (!canal_puede_entrar(canal_global, candidato))
            continue;  // bloqueado por política, probar el otro lado

        // Aceptado
        barco_t *b = sq_deq_max(q);
        b->state = READY;
        printf("[SJF] -> Barco %d (%s) autorizado (vel=%d, dir=%s)\n",
               b->id, b->nombre, b->velocidad,
               b->direccion == 0 ? "IZQ" : "DER");
        return b;
    }

    return NULL;  // ninguna dirección disponible este tick
}

/* ─── notify_done ─────────────────────────────────────── */
static void sjf_notify_done(barco_t *b)
{
    if (!b) return;

    b->state = DONE;

    if (en_canal > 0)
        en_canal--;

    printf("[SJF] Barco %d (%s) salió. en_canal=%d\n",
           b->id, b->nombre, en_canal);
}

/* ─── get_queue (para LEDs) ───────────────────────────── */
static int sjf_get_queue(int direccion, barco_t **out, int max)
{
    return sq_get_queue(direccion == 0 ? &q_left : &q_right, out, max);
}

/* ─── release — no-op en SJF ─────────────────────────── */
static void sjf_release(void) { }

/* ─── export ──────────────────────────────────────────── */
scheduler_t scheduler_sjf = {
    .init        = sjf_init,
    .next        = sjf_next,
    .release     = sjf_release,
    .enqueue     = sjf_enqueue,
    .notify_done = sjf_notify_done,
    .get_queue   = sjf_get_queue,
};