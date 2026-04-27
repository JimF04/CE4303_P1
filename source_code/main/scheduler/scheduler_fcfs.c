#include "scheduler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_Q 128

static barco_t *cola[BARCOS_MAX];
static int cola_len = 0;
static int fcfs_next_idx = 0;

static canal_t *g_canal;

static barco_t *q[MAX_Q];
static int q_head = 0;
static int q_tail = 0;

static void enqueue(barco_t *b)
{
    q[q_tail++] = b;
}

static barco_t *dequeue(void)
{
    if (q_head == q_tail) return NULL;
    return q[q_head++];
}

// ----- INIT -----
static void fcfs_init(canal_t *canal, const config_t *cfg)
{
    g_canal = canal;

    for (int i = 0; i < barcos_count(); i++)
        enqueue(barcos_get(i));
}

// ----- NEXT -----
static barco_t *fcfs_next(void)
{
    return dequeue();
}
// ----- EXPORTAR -----
scheduler_t scheduler_fcfs = {
    .init = fcfs_init,
    .next = fcfs_next
};