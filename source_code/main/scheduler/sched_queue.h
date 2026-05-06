#ifndef MAIN_SCHEDULER_SCHED_QUEUE_H_
#define MAIN_SCHEDULER_SCHED_QUEUE_H_


#include "../barcos/barco.h"

#define MAX_Q 64

/* ─── entrada de cola ─────────────────────────────────── */
typedef struct {
    barco_t *barco;
    int      valor;   // orden, prioridad, velocidad, etc. según algoritmo
} sched_entry_t;

typedef struct {
    sched_entry_t data[MAX_Q];
    int head, tail, size;
} sched_queue_t;

/* ─── init ───────────────────────────────────────────── */
static inline void sq_init(sched_queue_t *q)
{
    q->head = q->tail = q->size = 0;
}

/* ─── enqueue ─────────────────────────────────────────── */
static inline void sq_enq(sched_queue_t *q, barco_t *b, int valor)
{
    if (!b || q->size >= MAX_Q) return;
    q->data[q->tail].barco = b;
    q->data[q->tail].valor = valor;
    q->tail = (q->tail + 1) % MAX_Q;
    q->size++;
}

/* ─── dequeue FIFO ────────────────────────────────────── */
static inline barco_t *sq_deq(sched_queue_t *q)
{
    if (q->size == 0) return NULL;
    barco_t *b = q->data[q->head].barco;
    q->head = (q->head + 1) % MAX_Q;
    q->size--;

    return b;
}

/* ─── dequeue mayor valor ─────────────────────────────── */
static inline barco_t *sq_deq_max(sched_queue_t *q)
{
    if (q->size == 0) return NULL;

    int max_idx = q->head;
    int max_val = q->data[q->head].valor;

    for (int i = 1; i < q->size; i++) {
        int idx = (q->head + i) % MAX_Q;
        if (q->data[idx].valor > max_val) {
            max_val = q->data[idx].valor;
            max_idx = idx;
        }
    }

    barco_t *b = q->data[max_idx].barco;

    // Compactar: mover elementos hacia el head
    while (max_idx != q->head) {
        int prev = (max_idx - 1 + MAX_Q) % MAX_Q;
        q->data[max_idx] = q->data[prev];
        max_idx = prev;
    }
    q->head = (q->head + 1) % MAX_Q;
    q->size--;

    return b;
}

/* ─── dequeue menor valor ─────────────────────────────── */
static inline barco_t *sq_deq_min(sched_queue_t *q)
{
    if (q->size == 0) return NULL;

    int min_idx = q->head;
    int min_val = q->data[q->head].valor;

    for (int i = 1; i < q->size; i++) {
        int idx = (q->head + i) % MAX_Q;
        if (q->data[idx].valor < min_val) {
            min_val = q->data[idx].valor;
            min_idx = idx;
        }
    }

    barco_t *b = q->data[min_idx].barco;

    // Compactar
    while (min_idx != q->head) {
        int prev = (min_idx - 1 + MAX_Q) % MAX_Q;
        q->data[min_idx] = q->data[prev];
        min_idx = prev;
    }
    q->head = (q->head + 1) % MAX_Q;
    q->size--;

    return b;
}

/* ─── peek frente (FIFO) ──────────────────────────────── */
static inline int sq_peek_front(sched_queue_t *q)
{
    if (q->size == 0) return 0x7FFFFFFF;   // infinito
    return q->data[q->head].valor;
}

/* ─── peek mayor valor ────────────────────────────────── */
static inline int sq_peek_max(sched_queue_t *q)
{
    if (q->size == 0) return -1;
    int max = q->data[q->head].valor;
    for (int i = 1; i < q->size; i++) {
        int idx = (q->head + i) % MAX_Q;
        if (q->data[idx].valor > max)
            max = q->data[idx].valor;
    }
    return max;
}

/* ─── peek menor valor ────────────────────────────────── */
static inline int sq_peek_min(sched_queue_t *q)
{
    if (q->size == 0) return 0x7FFFFFFF;
    int min = q->data[q->head].valor;
    for (int i = 1; i < q->size; i++) {
        int idx = (q->head + i) % MAX_Q;
        if (q->data[idx].valor < min)
            min = q->data[idx].valor;
    }
    return min;
}

/* ─── exportar orden de cola (para LEDs) ─────────────── */
static inline int sq_get_queue(sched_queue_t *q, barco_t **out, int max)
{
    int count = 0;
    for (int i = 0; i < q->size && count < max; i++) {
        int idx = (q->head + i) % MAX_Q;
        out[count++] = q->data[idx].barco;
    }
    return count;
}

/* ─── peek barco frente (FIFO) ────────────────────────── */
static inline barco_t *sq_peek_barco(sched_queue_t *q)
{
    if (q->size == 0) return NULL;
    return q->data[q->head].barco;
}

static inline barco_t *sq_peek_barco_max(sched_queue_t *q)
{
    if (q->size == 0) return NULL;
    int max_idx = q->head;
    int max_val = q->data[q->head].valor;
    for (int i = 1; i < q->size; i++) {
        int idx = (q->head + i) % MAX_Q;
        if (q->data[idx].valor > max_val) {
            max_val = q->data[idx].valor;
            max_idx = idx;
        }
    }
    return q->data[max_idx].barco;
}

#endif /* MAIN_SCHEDULER_SCHED_QUEUE_H_ */
