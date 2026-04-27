#include "flow_policy.h"
#include "canal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════
   TICO
   ─ Sin control de flujo.
   ─ Garantiza no-colisión: si hay barcos de una dirección
     dentro, la contraria NO puede entrar.
   ─ Si solo hay barcos de un lado, pasan sin restricción.
   ═══════════════════════════════════════════════════════ */

static void tico_init(flow_policy_t *self, canal_t *c, const config_t *cfg)
{
    (void)cfg;
    self->state = NULL;
}

// Devuelve la dirección activa en el canal (-1 si vacío)
static int tico_dir_activa(canal_t *c)
{
    for (int i = 0; i < c->largo; i++) {
        if (c->slots[i] != NULL)
            return c->slots[i]->direccion;
    }
    return -1; // canal vacío
}

static int tico_allow_left(flow_policy_t *self, canal_t *c)
{
    (void)self;
    int dir = tico_dir_activa(c);
    // Puede entrar si el canal está vacío o ya van hacia la izquierda (dir=0)
    return (dir == -1 || dir == 0);
}

static int tico_allow_right(flow_policy_t *self, canal_t *c)
{
    (void)self;
    int dir = tico_dir_activa(c);
    return (dir == -1 || dir == 1);
}

static void tico_tick(flow_policy_t *self, canal_t *c)
{
    (void)self; (void)c;
}

/* ═══════════════════════════════════════════════════════
   FACTORY
   ═══════════════════════════════════════════════════════ */

flow_policy_t *flow_policy_create(const char *mode, const config_t *cfg)
{
    flow_policy_t *p = malloc(sizeof(flow_policy_t));
    if (!p) return NULL;

    if (strcmp(mode, "TICO") == 0) {
		p->init        = tico_init;
		p->allow_left  = tico_allow_left;
		p->allow_right = tico_allow_right;
		p->tick        = tico_tick;
		p->state       = NULL;
        return p;
    }

    // Default = TICO
    p->init        = tico_init;
    p->allow_left  = tico_allow_left;
    p->allow_right = tico_allow_right;
    p->tick        = tico_tick;
    p->state       = NULL;
    return p;
}