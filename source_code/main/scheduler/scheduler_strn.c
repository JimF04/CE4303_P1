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

    // ── CASO 1: actual existe pero no está en el canal ──
    if (actual != NULL && actual->pos_canal == -1) {

        // Verificar si la política lo permite ahora
        if (canal_puede_entrar(canal_global, actual))
            return actual;

        // Política bloquea: re-encolar y buscar otro lado
        printf("[STRN] Barco %d bloqueado por política, cediendo\n", actual->id);
        strn_enqueue(actual);
        actual = NULL;
        // caer al caso 2
    }

    int v_izq = sq_peek_max(&q_left);
    int v_der = sq_peek_max(&q_right);

    if (v_izq == -1 && v_der == -1 && actual == NULL)
        return NULL;

    // Orden de preferencia según dirección del canal
    int primera, segunda;
    if (dir_canal == 0) {
        primera = 0; segunda = -1;  // solo IZQ
    } else if (dir_canal == 1) {
        primera = 1; segunda = -1;  // solo DER
    } else {
        // Canal libre -> el más rápido global
        if (v_izq >= v_der) { primera = 0; segunda = 1; }
        else                 { primera = 1; segunda = 0; }
    }

    // Sin barco corriendo -> meter el mejor que pueda entrar
    if (actual == NULL) {
        int dirs[2] = { primera, segunda };
        for (int i = 0; i < 2; i++) {
            int d = dirs[i];
            if (d == -1) break;

            sched_queue_t *q = (d == 0) ? &q_left : &q_right;

            // Peek sin desencolar todavía
            barco_t *candidato = sq_peek_barco_max(q);
            if (!candidato || candidato->id == -1 || candidato->state == DONE) {
                sq_deq_max(q);  // limpiar entrada inválida
                continue;
            }

            // Consultar al canal si puede entrar ANTES de desencolar
            if (!canal_puede_entrar(canal_global, candidato))
                continue;  // bloqueado por política, probar el otro lado

            barco_t *b = sq_deq_max(q);
            b->state = READY;
            actual   = b;
            printf("[STRN] -> Barco %d entra\n", b->id);
            return b;
        }
        return NULL;
    }

    // Hay barco corriendo -> evaluar preemption
    // Buscar mejor candidato que pueda entrar en algún lado
    int mejor_v   = -1;
    int mejor_dir = -1;
    int dirs2[2]  = { primera, segunda };
    for (int i = 0; i < 2; i++) {
        int d = dirs2[i];
        if (d == -1) break;

        sched_queue_t *q = (d == 0) ? &q_left : &q_right;
        barco_t *candidato = sq_peek_barco_max(q);
        if (!candidato || candidato->id == -1) continue;

        // Solo considerar si la política lo permite
        if (!canal_puede_entrar(canal_global, candidato)) continue;

        int v = (d == 0) ? v_izq : v_der;
        if (v > mejor_v) { mejor_v = v; mejor_dir = d; }
    }

    if (actual->pos_canal >= 0 && mejor_v > actual->velocidad) {
        printf("[STRN] Preempt: barco %d (vel=%d) -> nuevo (vel=%d)\n",
               actual->id, actual->velocidad, mejor_v);

        preempt(actual);  // saca del canal y re-encola; actual = NULL

        sched_queue_t *q = (mejor_dir == 0) ? &q_left : &q_right;
        barco_t *b = sq_deq_max(q);
        if (b) {
            b->state = READY;
            actual   = b;
            printf("[STRN] -> Barco %d entra (preemptivo)\n", b->id);
        }
        return b;
    }

    return NULL; // sigue el actual
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