#include "scheduler.h"
#include "sched_queue.h"
#include <string.h>
#include <stdio.h>
#include "events.h"	

#define RR_QUANTUM 3   // por ejemplo

/* ─── colas por dirección ─────────────────────────────── */
static sched_queue_t q_left, q_right;

/* ─── estado del canal ────────────────────────────────── */
//static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro

static int orden_global;   // contador global de llegada

static barco_t *actual = NULL;
extern canal_t *canal_global;


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

    // Construir orden de preferencia igual que FCFS
    int primera, segunda;

    if (dir == 0) {
        primera = 0; segunda = -1;
    } else if (dir == 1) {
        primera = 1; segunda = -1;
    } else {
        // Canal libre: preferir IZQ por default en RR
        primera = 0; segunda = 1;
    }

    int dirs[2] = { primera, segunda };
    for (int i = 0; i < 2; i++) {
        int d = dirs[i];
        if (d == -1) break;

        sched_queue_t *q = (d == 0) ? &q_left : &q_right;
        if (sq_peek_front(q) == 0x7FFFFFFF) continue;

        barco_t *candidato = sq_peek_barco(q);
        if (!candidato || candidato->id == -1 || candidato->state == DONE) {
            sq_deq(q);  // limpiar inválido
            continue;
        }

        // Verificar política ANTES de desencolar
        if (!canal_puede_entrar(canal_global, candidato))
            continue;

        return sq_deq(q);
    }

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
		   
   	// Avisar al main_task que hay un barco esperando
   	canal_event_t evt = { .type = EVT_BARCO_LISTO, .data = b };
   	xQueueSend(event_queue, &evt, 0);
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

    // xSemaphoreTake(canal_global->mutex, portMAX_DELAY);

    canal_remover_barco(canal_global, b);

    // xSemaphoreGive(canal_global->mutex);

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

	// ── CASO 1: actual existe pero no está en el canal ──
    if (actual != NULL && actual->pos_canal == -1) {

        // Verificar si la política lo permite ahora
        if (canal_puede_entrar(canal_global, actual)) {
            printf("[RR] Reintentando barco %d (%s)\n",
                   actual->id, actual->nombre);
            return actual;
        }

        // Política lo bloquea: re-encolar y ceder al otro lado
        printf("[RR] Barco %d bloqueado por política, cediendo\n", actual->id);
        rr_enqueue(actual);
        actual   = NULL;
        rr_ticks = 0;

        barco_t *b = elegir_siguiente();
        if (!b) return NULL;

        actual = b;
        printf("[RR] Nuevo (cedido) -> Barco %d (%s) (dir=%s)\n",
               b->id, b->nombre,
               b->direccion == 0 ? "IZQ" : "DER");
        return b;
    }
	
	// ── CASO 2: No hay actual -> elegir nuevo ──
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
	
	// ── CASO 3: actual está en el canal -> contar quantum ──
    rr_ticks++;

    if (rr_ticks < RR_QUANTUM)
        return NULL;

    rr_ticks = 0;
    barco_t *viejo = actual;

    printf("[RR] Quantum terminado para barco %d\n", viejo->id);

    preempt_rr(viejo);  // actual = NULL después de esto

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