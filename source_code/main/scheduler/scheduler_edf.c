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
static void edf_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, b->velocidad);   // valor = valocidad

    printf("[EDF] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void edf_init(canal_t *canal, const config_t *cfg)
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

        edf_enqueue(b);
    }
}

//prepara al barco para quitarlo del canal

void preempt_edf(barco_t *b) 
{
    if (!b) return; //que sea un barco valido

    // xSemaphoreTake(canal_global->mutex, portMAX_DELAY); //protege el canal

    canal_remover_barco(canal_global, b); //llama a quitar el barco

    // xSemaphoreGive(canal_global->mutex); //lo libera

    b->state = READY; //lo devuelve a ready
    actual = NULL;
    en_canal = 0;

    // lo vuelve a meter a la cola 
    edf_enqueue(b);

    printf("[SCHED] Barco %d preempted\n", b->id);
}


/* ─── next ───────────────────────────────────────────── */
static barco_t *edf_next(void)
{
    int dir_canal = canal_global->direccion_actual;

    // prioridad mínima en cada cola (menor = más importante)
    int p_izq = sq_peek_min(&q_left);
    int p_der = sq_peek_min(&q_right);

    if (p_izq == -1 && p_der == -1)
        return NULL;

        //----------------------------------------
    // Elegir mejor dirección
    //----------------------------------------
    int mejor_dir = -1;
    int mejor_p   = -1;

    if (dir_canal == 0) {
        mejor_dir = (p_izq != -1) ? 0 : -1;
        mejor_p   = p_izq;
    }
    else if (dir_canal == 1) {
        mejor_dir = (p_der != -1) ? 1 : -1;
        mejor_p   = p_der;
    }
    else {
        // menor prioridad gana
        if (p_izq <= p_der) {
            mejor_dir = 0;
            mejor_p   = p_izq;
        } else {
            mejor_dir = 1;
            mejor_p   = p_der;
        }
    }

    if (mejor_dir == -1)
        return NULL;

    sched_queue_t *q =
        (mejor_dir == 0) ? &q_left : &q_right;

    barco_t *candidato = sq_peek_barco_min(q);

    if (!candidato)
        return NULL;

    if (!canal_puede_entrar(canal_global, candidato))
        return NULL;

    // Si hay espacio, entra directo

    if (!canal_lleno(canal_global)) {
        barco_t *b = sq_deq_min(q);

        b->state = READY;

        printf("[EDF] -> entra %d (prio=%d)\n",
               b->id, b->prioridad);

        return b;
    }

    // Canal lleno: buscar el peor

    barco_t *peor = canal_barco_max(canal_global, 1);

    if (!peor)
        return NULL;
    // Si candidato tiene MENOR prioridad, reemplaza
    if (candidato->prioridad < peor->prioridad) {

        printf("[EDF] Preempt %d (prio=%d) por %d (prio=%d)\n",
               peor->id,
               peor->prioridad,
               candidato->id,
               candidato->prioridad);

        preempt_edf(peor);

        barco_t *b = sq_deq_min(q);

        b->state = READY;

        return b;
    }

    return NULL;
}

/* ─── notify_done ─────────────────────────────────────── */
static void edf_notify_done(barco_t *b)
{
    if (!b) return;

    b->state = DONE;

    if (actual == b) {
        actual = NULL;
    }

    if (en_canal > 0)

    printf("[EDF] Barco %d salió\n", b->id);
}

/* ─── get_queue (para LEDs) ───────────────────────────── */
static int edf_get_queue(int direccion, barco_t **out, int max)
{
    return sq_get_queue(direccion == 0 ? &q_left : &q_right, out, max);
}

/* ─── release — no-op en edf ─────────────────────────── */
static void edf_release(void) { }

/* ─── export ──────────────────────────────────────────── */
scheduler_t scheduler_edf = {
    .init        = edf_init,
    .next        = edf_next,
    .release     = edf_release,
    .enqueue     = edf_enqueue,
    .notify_done = edf_notify_done,
    .get_queue   = edf_get_queue,
};