#include "flow_policy.h"
#include "canal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════
   EQUIDAD
   ─ Parametro W que indica cuantos barcos deben de pasar
     de cada lado.
   ─ Garantiza no-colisión: si hay barcos de una dirección
     dentro, la contraria NO puede entrar.
   ─ Se debe garantizar el flujo, en el lado en el que si 
     hayan barcos disponibles.
═══════════════════════════════════════════════════════ */

typedef struct {
    int dir_activa;   // 0=IZQ, 1=DER
    int pasados_w;    // cuántos han SALIDO en esta ronda
    int w;            // máximo por ronda
} equidad_state_t;

static equidad_state_t eq_state;

static void equidad_init(flow_policy_t *self, canal_t *c, const config_t *cfg)
{
    (void)c;
    eq_state.dir_activa = 0;          // arranca IZQ
    eq_state.pasados_w  = 0;
    eq_state.w          = cfg->canal.parametro_w;
    self->state         = &eq_state;

    printf("[EQUIDAD] W=%d, arranca IZQ\n", eq_state.w);
}

// Cuántos barcos hay esperando en una dirección
static int equidad_hay_barcos_dir(int dir)
{
    for (int i = 0; i < barcos_count(); i++) {
        barco_t *b = barcos_get(i);
        if (!b || b->id == -1) continue;
        if (b->direccion == dir &&
            b->state != DONE &&
            b->pos_canal == -1)   // no está ya adentro
            return 1;
    }
    return 0;
}

// Devuelve la dirección activa en el canal (-1 si vacío)
static int equidad_dir_activa_canal(canal_t *c)
{
    for (int i = 0; i < c->largo; i++)
        if (c->slots[i] != NULL)
            return c->slots[i]->direccion;
    return -1;
}

static int equidad_allow(flow_policy_t *self, canal_t *c, barco_t *b)
{
    equidad_state_t *s = (equidad_state_t *)self->state;
    int dir_canal = equidad_dir_activa_canal(c);

    // No-colisión: si hay barcos adentro de otra dirección, bloquear
    if (dir_canal != -1 && dir_canal != b->direccion)
        return 0;

    // Si el lado activo no tiene barcos esperando ni dentro, ceder al otro
    if (!equidad_hay_barcos_dir(s->dir_activa) && dir_canal == -1)
        return 1;

    // Solo permitir si coincide con la dirección activa de EQUIDAD
    if (b->direccion != s->dir_activa)
        return 0;

    return 1;
}

static void equidad_tick(flow_policy_t *self, canal_t *c)
{
    (void)self; (void)c;
} 

void equidad_notify_salio(flow_policy_t *self, int direccion)
{
    equidad_state_t *s = (equidad_state_t *)self->state;

    // Solo contar si salió del lado activo
    if (direccion != s->dir_activa)
        return;

    s->pasados_w++;
    printf("[EQUIDAD] Barco salió dir=%s, pasados=%d/%d\n",
           direccion == 0 ? "IZQ" : "DER",
           s->pasados_w, s->w);

    // ¿Cumplimos el W?
    if (s->pasados_w >= s->w) {
        int otro = 1 - s->dir_activa;

        if (equidad_hay_barcos_dir(otro)) {
            // Cambiar al otro lado
            s->dir_activa = otro;
            s->pasados_w  = 0;
            printf("[EQUIDAD] Cambio a %s (W cumplido)\n",
                   otro == 0 ? "IZQ" : "DER");
        } else {
            // Otro lado vacío: resetear W y continuar mismo lado
            s->pasados_w = 0;
            printf("[EQUIDAD] Otro lado vacío, continúa %s\n",
                   s->dir_activa == 0 ? "IZQ" : "DER");
        }
    }
}


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

static int tico_allow(flow_policy_t *self, canal_t *c, barco_t *b)
{
    (void)self;
    int dir = tico_dir_activa(c);

    // canal vacío -> puede entrar cualquiera
    if (dir == -1) return 1;

    // si la dirección coinciden -> permitir
    if (dir == b->direccion) return 1;

    return 0;
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
		p->allow       = tico_allow;
		p->tick        = tico_tick;
		p->state       = NULL;
		p->notify_salio = NULL;
        return p;
    }
	
	if (strcmp(mode, "EQUIDAD") == 0) {
		p->init        = equidad_init;
		p->allow       = equidad_allow;
		p->tick        = equidad_tick;
		p->state       = NULL;
		p->notify_salio = equidad_notify_salio;
	    return p;
	}

    // Default = TICO
    p->init        = tico_init;
	p->allow       = tico_allow;
    p->tick        = tico_tick;
    p->state       = NULL;
	p->notify_salio = NULL;
    return p;
}