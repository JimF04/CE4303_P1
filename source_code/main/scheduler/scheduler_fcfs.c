#include "scheduler.h"
#include <string.h>
#include <stdio.h>

#define MAX_Q 64

/* ─── entrada de cola con timestamp ──────────────────── */
typedef struct {
    barco_t *barco;
    int      orden;   // orden global de llegada (menor = llegó primero)
} entrada_t;

static entrada_t q_left [MAX_Q];
static entrada_t q_right[MAX_Q];
static int ql_head, ql_tail, ql_size;
static int qr_head, qr_tail, qr_size;

static int orden_global = 0;   // contador global de llegada

/* ─── estado del canal ────────────────────────────────── */
static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro

/* ─── helpers ─────────────────────────────────────────── */
static void _enq(entrada_t *q, int *tail, int *size, barco_t *b)
{
    if (*size >= MAX_Q) return;
    q[*tail].barco = b;
    q[*tail].orden = orden_global++;
    *tail = (*tail + 1) % MAX_Q;
    (*size)++;
}

static barco_t *_deq(entrada_t *q, int *head, int *size)
{
    if (*size == 0) return NULL;
    barco_t *b = q[*head].barco;
    *head = (*head + 1) % MAX_Q;
    (*size)--;
    return b;
}

static int _peek_orden(entrada_t *q, int head, int size)
{
    if (size == 0) return 0x7FFFFFFF;   // infinito
    return q[head].orden;
}

/* ─── enqueue público ─────────────────────────────────── */
static void fcfs_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    if (b->direccion == 0)
        _enq(q_left,  &ql_tail, &ql_size, b);
    else
        _enq(q_right, &qr_tail, &qr_size, b);

    printf("[FCFS] Barco %d (%s) encolado (orden=%d, dir=%s)\n",
           b->id, b->nombre, orden_global - 1,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void fcfs_init(canal_t *canal, const config_t *cfg)
{
    (void)canal; (void)cfg;

    ql_head = ql_tail = ql_size = 0;
    qr_head = qr_tail = qr_size = 0;
    orden_global = 0;
    canal_dir    = -1;
    en_canal     =  0;

    for (int i = 0; i < barcos_count(); i++)
        fcfs_enqueue(barcos_get(i));
}

/* ─── next ───────────────────────────────────────────── */
static barco_t *fcfs_next(void)
{
    // FCFS puro: si hay alguien dentro, no entra nadie más
    if (en_canal > 0)
        return NULL;

    // Canal vacío: elegir dirección por orden de llegada
    canal_dir = -1;

    int orden_izq = _peek_orden(q_left,  ql_head, ql_size);
    int orden_der = _peek_orden(q_right, qr_head, qr_size);

    if (orden_izq == 0x7FFFFFFF && orden_der == 0x7FFFFFFF)
        return NULL;

    canal_dir = (orden_izq <= orden_der) ? 0 : 1;

    printf("[FCFS] Canal libre -> dirección elegida: %s (orden izq=%d, der=%d)\n",
           canal_dir == 0 ? "IZQ" : "DER", orden_izq, orden_der);

    barco_t *b = NULL;
    if (canal_dir == 0) b = _deq(q_left,  &ql_head, &ql_size);
    else                b = _deq(q_right, &qr_head, &qr_size);

    if (b) {
        b->state = RUNNING;
        en_canal++;
        printf("[FCFS] -> Barco %d (%s) autorizado (en_canal=%d)\n",
               b->id, b->nombre, en_canal);
    }

    return b;
}

/* ─── notify_done ─────────────────────────────────────── */
static void fcfs_notify_done(barco_t *b)
{
    if (!b) return;
    b->state = DONE;
    if (en_canal > 0) en_canal--;
    printf("[FCFS] Barco %d (%s) salió. en_canal=%d\n",
           b->id, b->nombre, en_canal);
}

/* ─── fcfs_get_queue ─────────────────────────────────────── */
static int fcfs_get_queue(int direccion, barco_t **out, int max)
{
    entrada_t *q    = (direccion == 0) ? q_left  : q_right;
    int        head = (direccion == 0) ? ql_head : qr_head;
    int        size = (direccion == 0) ? ql_size : qr_size;

    int count = 0;
    for (int i = 0; i < size && count < max; i++) {
        int idx = (head + i) % MAX_Q;
        out[count++] = q[idx].barco;
    }
    return count;
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