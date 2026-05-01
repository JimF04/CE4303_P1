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

static barco_t *actual = NULL;
extern canal_t *canal_global;
extern SemaphoreHandle_t canal_mutex;



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

    xSemaphoreTake(canal_mutex, portMAX_DELAY); //protege el canal

    canal_remover_barco(canal_global, b); //llama a quitar el barco

    xSemaphoreGive(canal_mutex); //lo libera

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
    // Ver mejor candidato en cola
    int v_izq = sq_peek_max(&q_left);
    int v_der = sq_peek_max(&q_right);

    barco_t *mejor = NULL;
    int mejor_dir = -1;
    int mejor_v   = -1;

    if (v_izq == -1 && v_der == -1) {
        return NULL;
    }

    if (v_izq >= v_der) {
        mejor_dir = 0;
        mejor_v   = v_izq;
    } else {
        mejor_dir = 1;
        mejor_v   = v_der;
    }

    // no hay nadie corriendo
    if (actual == NULL) {

        mejor = (mejor_dir == 0)
                ? sq_deq_max(&q_left)
                : sq_deq_max(&q_right);

        if (mejor) {
            mejor->state = RUNNING;
            actual = mejor;
            en_canal = 1;
            canal_dir = mejor->direccion;

            printf("[SRTN] -> Barco %d entra (sin competencia)\n", mejor->id);
        }

        return mejor;
    }

    //hay alguien corriendo → evaluar preemption
    if (mejor_v > actual->velocidad) {

        printf("[SRTN] Preempt: %d -> %d\n", actual->id, mejor_v);

        // sacar actual del canal
        preempt(actual);

        // sacar nuevo de la cola
        mejor = (mejor_dir == 0)
                ? sq_deq_max(&q_left)
                : sq_deq_max(&q_right);

        if (mejor) {
            mejor->state = RUNNING;
            actual = mejor;
            canal_dir = mejor->direccion;

            printf("[SRTN] -> Barco %d entra (preemptivo)\n", mejor->id);
        }

        return mejor;
    }

    //nadie mejor → sigue el actual
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
        en_canal--;

    if (en_canal == 0)
        canal_dir = -1;

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