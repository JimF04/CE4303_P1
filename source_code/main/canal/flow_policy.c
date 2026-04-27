#include "flow_policy.h"
#include <stdlib.h>
#include <string.h>

/* ===== LEETRERO ===== */
typedef struct {
    int dir;
    int counter;
} letrero_state_t;

/* ===== EQUIDAD ===== */
typedef struct {
    int W;
    int count;
    int dir;
} equidad_state_t;

/* =========================
   LETRERO
   ========================= */

static void letrero_init(flow_policy_t *self, canal_t *c, const config_t *cfg)
{
    letrero_state_t *s = malloc(sizeof(letrero_state_t));
    s->dir = 0;
    s->counter = 0;
    self->state = s;
}

static int letrero_left(flow_policy_t *self, canal_t *c)
{
    return ((letrero_state_t*)self->state)->dir == 0;
}

static int letrero_right(flow_policy_t *self, canal_t *c)
{
    return ((letrero_state_t*)self->state)->dir == 1;
}

static void letrero_tick(flow_policy_t *self, canal_t *c)
{
    letrero_state_t *s = self->state;
    s->counter++;

    if (s->counter >= 10) {
        s->dir ^= 1;
        s->counter = 0;
    }
}

/* =========================
   EQUIDAD
   ========================= */

static void equidad_init(flow_policy_t *self, canal_t *c, const config_t *cfg)
{
    equidad_state_t *s = malloc(sizeof(equidad_state_t));
    s->W = cfg->canal.parametro_w;
    s->count = 0;
    s->dir = 0;
    self->state = s;
}

static int equidad_left(flow_policy_t *self, canal_t *c)
{
    equidad_state_t *s = self->state;

    if (s->dir == 0) return 1;
    return s->count < s->W;
}

static int equidad_right(flow_policy_t *self, canal_t *c)
{
    equidad_state_t *s = self->state;

    if (s->dir == 1) return 1;
    return s->count < s->W;
}

static void equidad_tick(flow_policy_t *self, canal_t *c)
{
    equidad_state_t *s = self->state;

    s->count++;

    if (s->count >= s->W) {
        s->count = 0;
        s->dir ^= 1;
    }
}

/* =========================
   TICO
   ========================= */

static void tico_init(flow_policy_t *self, canal_t *c, const config_t *cfg)
{
    self->state = NULL;
}

static int tico_left(flow_policy_t *self, canal_t *c)
{
    return 1;
}

static int tico_right(flow_policy_t *self, canal_t *c)
{
    return 1;
}

static void tico_tick(flow_policy_t *self, canal_t *c)
{
}

/* =========================
   FACTORY
   ========================= */

flow_policy_t *flow_policy_create(const char *mode, const config_t *cfg)
{
    flow_policy_t *p = malloc(sizeof(flow_policy_t));

    if (strcmp(mode, "letrero") == 0) {
        p->init = letrero_init;
        p->allow_left = letrero_left;
        p->allow_right = letrero_right;
        p->tick = letrero_tick;
        return p;
    }

    if (strcmp(mode, "equidad") == 0) {
        p->init = equidad_init;
        p->allow_left = equidad_left;
        p->allow_right = equidad_right;
        p->tick = equidad_tick;
        return p;
    }

    /* default = tico */
    p->init = tico_init;
    p->allow_left = tico_left;
    p->allow_right = tico_right;
    p->tick = tico_tick;

    return p;
}