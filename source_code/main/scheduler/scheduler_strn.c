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

static barco_t *actual = NULL;
extern canal_t *canal_global;



/* ─── enqueue público ─────────────────────────────────── */
static void strn_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, b->velocidad);   // valor = valocidad

    printf("[strn] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void strn_init(canal_t *canal, const config_t *cfg)
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

        strn_enqueue(b);
    }
}

//prepara al barco para quitarlo del canal

void preempt(barco_t *b) 
{
    if (!b) return; //que sea un barco valido

    // xSemaphoreTake(canal_global->mutex, portMAX_DELAY); //protege el canal

    canal_remover_barco(canal_global, b); //llama a quitar el barco

    // xSemaphoreGive(canal_global->mutex); //lo libera

    b->state = READY; //lo devuelve a ready
    actual = NULL;
    en_canal = 0;

    // lo vuelve a meter a la cola 
    strn_enqueue(b);

    printf("[SCHED] Barco %d preempted\n", b->id);
}


/* ─── next ───────────────────────────────────────────── */
static barco_t *strn_next(void)
{
    int dir_canal = canal_global->direccion_actual;

    int v_izq = sq_peek_max(&q_left);
    int v_der = sq_peek_max(&q_right);

    if (v_izq == -1 && v_der == -1)
        return NULL;

    // elegir mejor dirección
    int mejor_dir = -1;
    int mejor_v   = -1;

    if (dir_canal == 0) {
        mejor_dir = (v_izq != -1) ? 0 : -1;
        mejor_v   = v_izq;
    }
    else if (dir_canal == 1) {
        mejor_dir = (v_der != -1) ? 1 : -1;
        mejor_v   = v_der;
    }
    else {
        if (v_izq >= v_der) {
            mejor_dir = 0;
            mejor_v   = v_izq;
        } else {
            mejor_dir = 1;
            mejor_v   = v_der;
        }
    }

    if (mejor_dir == -1)
        return NULL;

    sched_queue_t *q =
        (mejor_dir == 0) ? &q_left : &q_right;

    barco_t *candidato = sq_peek_barco_max(q);

    if (!candidato)
        return NULL;

    if (!canal_puede_entrar(canal_global, candidato))
        return NULL;

    
    // hay espacio en canal
    if (!canal_lleno(canal_global)) {
        barco_t *b = sq_deq_max(q);

        b->state = READY;

        printf("[STRN] -> entra %d\n", b->id);

        return b;
    }

   //canal lleno -> comparar con el peor
    barco_t *peor = canal_barco_min(canal_global, 0);

    if (!peor)
        return NULL;

    if (candidato->velocidad > peor->velocidad) {

        printf("[STRN] Preempt %d (vel=%d) por %d (vel=%d)\n",
               peor->id,
               peor->velocidad,
               candidato->id,
               candidato->velocidad);

        preempt(peor);

        barco_t *b = sq_deq_max(q);

        b->state = READY;

        return b;
    }

    return NULL;
}

/* ─── notify_done ─────────────────────────────────────── */
static void strn_notify_done(barco_t *b)
{
    if (!b) return;

    b->state = DONE;

    if (actual == b) {
        actual = NULL;
    }

    if (en_canal > 0)

    printf("[SRTN] Barco %d salió\n", b->id);
}

/* ─── get_queue (para LEDs) ───────────────────────────── */
static int strn_get_queue(int direccion, barco_t **out, int max)
{
    return sq_get_queue(direccion == 0 ? &q_left : &q_right, out, max);
}

/* ─── release — no-op en strn ─────────────────────────── */
static void strn_release(void) { }

/* ─── export ──────────────────────────────────────────── */
scheduler_t scheduler_strn = {
    .init        = strn_init,
    .next        = strn_next,
    .release     = strn_release,
    .enqueue     = strn_enqueue,
    .notify_done = strn_notify_done,
    .get_queue   = strn_get_queue,
};