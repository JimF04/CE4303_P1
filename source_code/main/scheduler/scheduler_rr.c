#include "scheduler.h"
#include "sched_queue.h"
#include <string.h>
#include <stdio.h>

#define RR_QUANTUM 3   // por ejemplo

/* ─── colas por dirección ─────────────────────────────── */
static sched_queue_t q_left, q_right;

/* ─── estado del canal ────────────────────────────────── */
//static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro

static int orden_global;   // contador global de llegada

static barco_t *actual = NULL;
extern canal_t *canal_global;
extern SemaphoreHandle_t canal_mutex;

static int rr_ticks = 0;

/* ─── helper: elige de la cola correcta según canal ──────
   Regla:
   - Canal activo IZQ (0) -> RR en q_left
   - Canal activo DER (1) -> RR en q_right
   - Canal libre    (-1)  -> por default IZQ; si no hay, DER
────────────────────────────────────────────────────────── */
static barco_t *elegir_siguiente(void)
{
    int dir = canal_global->direccion_actual;

    if (dir == 0) {
        // Canal en dirección IZQ -> solo sacar de q_left
        return (sq_peek_front(&q_left) != 0x7FFFFFFF)
               ? sq_deq(&q_left) : NULL;
    }

    if (dir == 1) {
        // Canal en dirección DER -> solo sacar de q_right
        return (sq_peek_front(&q_right) != 0x7FFFFFFF)
               ? sq_deq(&q_right) : NULL;
    }

    // Canal libre: preferir IZQ por default
    if (sq_peek_front(&q_left) != 0x7FFFFFFF)
        return sq_deq(&q_left);

    if (sq_peek_front(&q_right) != 0x7FFFFFFF)
        return sq_deq(&q_right);

    return NULL;
}


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

	// ── CASO 1: actual existe pero fue rechazado por el canal ──
    // pos_canal == -1 significa que main no pudo insertarlo
    if (actual != NULL && actual->pos_canal == -1) {
        int dir_canal = canal_global->direccion_actual;

        // Si el canal está ocupado en dirección contraria, esperar
        if (dir_canal != -1 && dir_canal != actual->direccion) {
            printf("[RR] Barco %d esperando cambio de dirección\n",
                   actual->id);
            return NULL;
        }

        // Canal libre o misma dirección: reintentar sin contar ticks
        printf("[RR] Reintentando barco %d (%s)\n",
               actual->id, actual->nombre);
        return actual;
    }
	
	// ── CASO 2: No hay actual -> elegir nuevo ──────────────────
    if (actual == NULL) {
        barco_t *b = elegir_siguiente();
        if (!b) return NULL;

        b->state = READY;
        actual   = b;
        rr_ticks = 0;

        printf("[RR] Nuevo -> Barco %d (%s) (dir=%s)\n",
               b->id, b->nombre,
               b->direccion == 0 ? "IZQ" : "DER");

        return b;
    }
	
	// ── CASO 3: actual está en el canal -> contar quantum ──────
    rr_ticks++;

    if (rr_ticks < RR_QUANTUM)
        return NULL;

    // Quantum agotado: rotar
    rr_ticks = 0;
    barco_t *viejo = actual;

    printf("[RR] Quantum terminado para barco %d\n", viejo->id);

    preempt_rr(viejo); // lo saca del canal y re-encola; actual = NULL

    // Elegir siguiente de la cola correcta (respeta dirección)
    barco_t *nuevo = elegir_siguiente();
    if (!nuevo) {
        actual = NULL;
        return NULL;
    }

    nuevo->state = READY;
    actual       = nuevo;

    printf("[RR] -> Barco %d (rotación, dir=%s)\n",
           nuevo->id, nuevo->direccion == 0 ? "IZQ" : "DER");

    return nuevo;
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