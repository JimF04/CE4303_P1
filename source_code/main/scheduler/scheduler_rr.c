#include "scheduler.h"
#include "sched_queue.h"
#include <string.h>
#include <stdio.h>

#define RR_QUANTUM 3   // por ejemplo

/* ─── colas por dirección ─────────────────────────────── */
static sched_queue_t q_left, q_right;

/* ─── estado del canal ────────────────────────────────── */
static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro

static int orden_global;   // contador global de llegada

static barco_t *actual = NULL;
extern canal_t *canal_global;
extern SemaphoreHandle_t canal_mutex;

static int rr_ticks = 0;


/* ─── enqueue público ─────────────────────────────────── */
static void rr_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    sched_queue_t *q = (b->direccion == 0) ? &q_left : &q_right;
    sq_enq(q, b, orden_global++);   // valor = orden de llegada

    printf("[rr] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void rr_init(canal_t *canal, const config_t *cfg)
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

        rr_enqueue(b);
    }
}

//lo mismo que el srnt
void preempt_rr(barco_t *b)
{
    if (!b) return;

    xSemaphoreTake(canal_mutex, portMAX_DELAY);

    canal_remover_barco(canal_global, b);

    xSemaphoreGive(canal_mutex);

    b->state = READY;
    actual = NULL;
    en_canal = 0;

    rr_enqueue(b);

    printf("[SCHED] Barco %d preempt_rred\n", b->id);
}


/* ─── next ───────────────────────────────────────────── */


static barco_t *rr_next(void)
{
    //no hay barco esperando, no hay siguiente
    if (q_left.size == 0 && q_right.size == 0 && actual == NULL) {
        return NULL;
    }

    //no hay barco en el canal
    if (actual == NULL) {

        int orden_izq = sq_peek_front(&q_left); //sigue la derecha?
        int orden_der = sq_peek_front(&q_right); //sigue la izquierda

        if (orden_izq == 0x7FFFFFFF && orden_der == 0x7FFFFFFF)
            return NULL;

        canal_dir = (orden_izq <= orden_der) ? 0 : 1;

        printf("[RR-FCFS] Canal libre -> dirección: %s (izq=%d, der=%d)\n",
               canal_dir == 0 ? "IZQ" : "DER", orden_izq, orden_der);

        barco_t *b = (canal_dir == 0)
            ? sq_deq(&q_left)
            : sq_deq(&q_right);

        if (b) {
            b->state = RUNNING; //lo ponemos a correr
            actual = b; //actual es el que va a entrar
            en_canal = 1; //hay uno en canal
            rr_ticks = 0; //para contar el quatum

            printf("[RR-FCFS] -> Barco %d (%s) entra\n",
                   b->id, b->nombre);
        }

        return b;
    }

    //si ya esta adentro
    rr_ticks++; //se suma al quatum

    if (rr_ticks < RR_QUANTUM) { //se le acabo el quatum?
        return NULL;
    }

    // se le acabo el quatum entonces rotamos de barco
    rr_ticks = 0; //se vuelve 0

    barco_t *viejo = actual; //el actual se vuelve el pasado

    printf("[RR-FCFS] Quantum terminado para barco %d\n", viejo->id);

    //lo rotas al final
    preempt_rr(viejo); //quitamos al viejo del canal

    //   elegir siguiente por FCFS otra vez
    int orden_izq = sq_peek_front(&q_left);
    int orden_der = sq_peek_front(&q_right);

    if (orden_izq == 0x7FFFFFFF && orden_der == 0x7FFFFFFF) {
        actual = NULL;
        return NULL;
    }

    canal_dir = (orden_izq <= orden_der) ? 0 : 1;

    barco_t *nuevo = (canal_dir == 0)
        ? sq_deq(&q_left)
        : sq_deq(&q_right);

    if (nuevo) {
        nuevo->state = RUNNING;
        actual = nuevo;
        en_canal = 1;

        printf("[RR-FCFS] -> Barco %d entra (rotación)\n", nuevo->id);
    }

    return nuevo; //se retorna el nuevo barco que va a correr
}

/* ─── notify_done ─────────────────────────────────────── */
static void rr_notify_done(barco_t *b)
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
static int rr_get_queue(int direccion, barco_t **out, int max)
{
    return sq_get_queue(direccion == 0 ? &q_left : &q_right, out, max);
}

/* ─── release — no-op en rr ─────────────────────────── */
static void rr_release(void) { }

/* ─── export ──────────────────────────────────────────── */
scheduler_t scheduler_rr = {
    .init        = rr_init,
    .next        = rr_next,
    .release     = rr_release,
    .enqueue     = rr_enqueue,
    .notify_done = rr_notify_done,
    .get_queue   = rr_get_queue,
};