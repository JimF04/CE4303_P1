#include "canal.h"
#include "flow_policy.h"
#include <stdio.h>
#include <string.h>

// Setear parametros del canal con el config
void canal_init(canal_t *c, const config_t *cfg)
{
    c->largo = cfg->canal.largo;

    c->direccion_actual = 0;
	
    c->ocupacion = 0;
	
    c->barcos_pasados = 0;

    for (int i = 0; i < c->largo; i++)
        c->slots[i] = NULL;

    c->policy = flow_policy_create(cfg->canal.metodo_flujo, cfg);
    c->policy->init(c->policy, c, cfg);
}

// Metodo para avanzar los barcos 
void canal_avanzar(canal_t *c)
{
    barco_t *nuevo_estado[MAX_LARGO];
    memset(nuevo_estado, 0, sizeof(nuevo_estado));

    // Primero pasar los que van a la DERECHA (dir=0): procesar de derecha a izquierda
    for (int i = c->largo - 1; i >= 0; i--) {
        barco_t *b = c->slots[i];
        if (!b || b->direccion != 0) continue;

        int nueva = i + b->velocidad;  // dir=0 -> +velocidad

        if (nueva >= c->largo) {
            b->state     = DONE;
            b->pos_canal = -1;
            c->ocupacion--;
            printf("[CANAL] Barco %d (%s) salió del canal\n", b->id, b->nombre);
            continue;
        }

        if (nuevo_estado[nueva] == NULL) {
            nuevo_estado[nueva] = b;
            b->pos_canal = nueva;
        } else {
            nuevo_estado[i] = b;  // bloqueado, se queda
        }
    }

    // Luego los que van a la IZQUIERDA (dir=1): procesar de izquierda a derecha
    for (int i = 0; i < c->largo; i++) {
        barco_t *b = c->slots[i];
        if (!b || b->direccion != 1) continue;

        int nueva = i - b->velocidad;  // dir=1 -> -velocidad

        if (nueva < 0) {
            b->state     = DONE;
            b->pos_canal = -1;
            c->ocupacion--;
            printf("[CANAL] Barco %d (%s) salió del canal\n", b->id, b->nombre);
            continue;
        }

        if (nuevo_estado[nueva] == NULL) {
            nuevo_estado[nueva] = b;
            b->pos_canal = nueva;
        } else {
            nuevo_estado[i] = b;  // bloqueado, se queda
        }
    }

    memcpy(c->slots, nuevo_estado, sizeof(c->slots));
    c->policy->tick(c->policy, c);
}

// Metodo para insertar nuevo barco al canal
void canal_insertar(canal_t *c, barco_t *b)
{
    if (!b) return;
    if (b->state != RUNNING && b->state != READY) return;

    // Toda la lógica de "quién puede entrar" la decide flow_policy
    if (b->direccion == 0 && !c->policy->allow_left(c->policy, c))  return;
    if (b->direccion == 1 && !c->policy->allow_right(c->policy, c)) return;

    int entrada = (b->direccion == 0) ? 0 : (c->largo - 1);

    if (c->slots[entrada] == NULL) {
        c->slots[entrada] = b;
        b->pos_canal = entrada;
        c->ocupacion++;
    }
}

// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b)
{
    if (!b) return 0;

    int dir = (b->direccion == 0) ? +1 : -1;
    int entrada = (b->direccion == 0) ? 0 : (c->largo - 1);

    // revisar TODO el camino según velocidad
    for (int i = 0; i <= b->velocidad; i++) {

        int idx = entrada + dir * i;

        if (idx < 0 || idx >= c->largo)
            return 0;

        if (c->slots[idx] != NULL)
            return 0;
    }

    return 1;
}

void canal_print(canal_t *c)
{
    printf("[");

    for (int i = 0; i < c->largo; i++) {
        if (c->slots[i] == NULL)
            printf(".");
        else
            printf("%d", c->slots[i]->id);
    }

    printf("]\n");
}