#include "scheduler.h"
#include <string.h>
#include <stdio.h>

#define MAX_Q 64

/* ─── entrada de cola con timestamp ──────────────────── */
typedef struct {
    barco_t *barco;
    int velocidad;
} entrada_t;

static entrada_t q_left [MAX_Q];
static entrada_t q_right[MAX_Q];
static int ql_head, ql_tail, ql_size;
static int qr_head, qr_tail, qr_size;

static int orden_global = 0;   // contador global de llegada

/* ─── estado del canal ────────────────────────────────── */
static int canal_dir;   // dirección activa (-1 = libre)
static int en_canal;    // barcos físicamente dentro

/*---------------------el que es mas rapido----------------*/


static int _peek_velocidad(entrada_t *q, int head, int size)
{
    if (size == 0) return -1; // o infinito negativo

    int max = -1;
    for (int i = 0; i < size; i++) {
        int idx = (head + i) % MAX_Q;
        if (q[idx].velocidad > max)
            max = q[idx].velocidad;
    }
    return max;
}


//deque del mas rapido
static barco_t *_deq_max(entrada_t *q, int *head, int *size)
{
    if (*size == 0) return NULL;

    int max_idx = *head;
    int max_vel = q[*head].velocidad;

    for (int i = 1; i < *size; i++) {
        int idx = (*head + i) % MAX_Q;
        if (q[idx].velocidad > max_vel) {
            max_vel = q[idx].velocidad;
            max_idx = idx;
        }
    }

    barco_t *b = q[max_idx].barco;

    // mover elementos (compactar)
    while (max_idx != *head) {
        int prev = (max_idx - 1 + MAX_Q) % MAX_Q;
        q[max_idx] = q[prev];
        max_idx = prev;
    }

    *head = (*head + 1) % MAX_Q;
    (*size)--;

    return b;
}




/* ─── helpers ─────────────────────────────────────────── */
static void _enq(entrada_t *q, int *tail, int *size, barco_t *b)
{
    if (*size >= MAX_Q) return;

    q[*tail].barco = b;
    q[*tail].velocidad = b->velocidad;

    *tail = (*tail + 1) % MAX_Q;
    (*size)++;
}


/* ─── enqueue público ─────────────────────────────────── */
static void sfj_enqueue(barco_t *b)
{
    if (!b) return;
    b->state = READY;

    if (b->direccion == 0)
        _enq(q_left,  &ql_tail, &ql_size, b);
    else
        _enq(q_right, &qr_tail, &qr_size, b);

    printf("[SJF] Barco %d (%s) encolado (velocidad=%d, dir=%s)\n",
           b->id, b->nombre, b->velocidad,
           b->direccion == 0 ? "IZQ" : "DER");
}

/* ─── init ───────────────────────────────────────────── */
static void sfj_init(canal_t *canal, const config_t *cfg)
{
    (void)canal; (void)cfg;

    ql_head = ql_tail = ql_size = 0;
    qr_head = qr_tail = qr_size = 0;
    orden_global = 0;
    canal_dir    = -1;
    en_canal     =  0;

    for (int i = 0; i < barcos_count(); i++)
        sfj_enqueue(barcos_get(i));
}

/* ─── next ───────────────────────────────────────────── */
static barco_t *sfj_next(void)
{

    if (en_canal > 0)
        return NULL;


    int vel_izq = _peek_velocidad(q_left,  ql_head, ql_size);
    int vel_der = _peek_velocidad(q_right, qr_head, qr_size);

    // Si ambas colas están vacías
    if (vel_izq == -1 && vel_der == -1)
        return NULL;

    barco_t *b = NULL;


    if (vel_izq > vel_der) {
        b = _deq_max(q_left, &ql_head, &ql_size);
    }
    else if (vel_der > vel_izq) {
        b = _deq_max(q_right, &qr_head, &qr_size);
    }
    else {

        b = _deq_max(q_left, &ql_head, &ql_size);
    }

    if (b) {
        b->state = RUNNING;
        en_canal++;

        canal_dir = b->direccion;

        printf("[PRIORIDAD] Canal libre -> dirección elegida: %s (vel izq=%d, der=%d)\n",
               canal_dir == 0 ? "IZQ" : "DER", vel_izq, vel_der);

        printf("[PRIORIDAD] -> Barco %d (%s) autorizado (en_canal=%d)\n",
               b->id, b->nombre, en_canal);
    }

    return b;
}

/* ─── notify_done ─────────────────────────────────────── */
static void sfj_notify_done(barco_t *b)
{
    if (!b) return;
    b->state = DONE;
    if (en_canal > 0) en_canal--;
    printf("[FCFS] Barco %d (%s) salió. en_canal=%d\n",
           b->id, b->nombre, en_canal);
}

static int sjf_get_queue(int direccion, barco_t **out, int max)
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
scheduler_t scheduler_sjf = {
    .init        = sfj_init,
    .next        = sfj_next,
    .release     = fcfs_release,
    .enqueue     = sfj_enqueue,
    .notify_done = sfj_notify_done,
	.get_queue   = sjf_get_queue,
};
