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



void canal_mover_barco(canal_t *c, barco_t *b)
{
    if (b->pos_canal < 0) return;

    int actual = b->pos_canal;
    int nueva;

    if (b->direccion == 0)
        nueva = actual + b->velocidad;
    else
        nueva = actual - b->velocidad;

    // SALE DEL CANAL
    if (nueva >= c->largo || nueva < 0) {
        c->slots[actual] = NULL;
        b->pos_canal = -1;
        b->state = DONE;
        c->ocupacion--;

        printf("[BARCO] %d salió del canal\n", b->id);
        return;
    }

    // COLISIÓN → se queda
    if (c->slots[nueva] != NULL) {
        return;
    }

    // MOVER
    c->slots[actual] = NULL;
    c->slots[nueva] = b;
    b->pos_canal = nueva;
}

// Metodo para insertar nuevo barco al canal
void canal_insertar(canal_t *c, barco_t *b)
{
    if (!b) return;
    if (b->state != RUNNING && b->state != READY) return;

    if (b->direccion == 0 && !c->policy->allow_left(c->policy, c))  return;
    if (b->direccion == 1 && !c->policy->allow_right(c->policy, c)) return;

    // La entrada ya considera la velocidad:
    // dir=0 (izq->der): entra en slot (velocidad - 1) porque avanzó vel slots desde -1
    // dir=1 (der->izq): entra en slot (largo - velocidad) por el mismo motivo
    int entrada;
    if (b->direccion == 0)
        entrada = b->velocidad - 1;
    else
        entrada = c->largo - b->velocidad;

    if (entrada < 0 || entrada >= c->largo) return;

    if (c->slots[entrada] == NULL) {
        c->slots[entrada] = b;
        b->pos_canal = entrada;
        c->ocupacion++;
        printf("[CANAL] Barco %d (%s) insertado en slot %d\n",
               b->id, b->nombre, entrada);
    }
}


// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b)
{
    if (!b) return 0;

    int dir = (b->direccion == 0) ? +1 : -1;

    // Punto de entrada ajustado a velocidad
    int entrada;
    if (b->direccion == 0)
        entrada = b->velocidad - 1;
    else
        entrada = c->largo - b->velocidad;

    if (entrada < 0 || entrada >= c->largo) return 0;

    // Revisar desde la entrada hasta vel pasos adelante
    // (el espacio que necesita para moverse el primer tick)
    for (int i = 0; i <= b->velocidad; i++) {
        int idx = entrada + dir * i;

        if (idx < 0 || idx >= c->largo) return 0;
        if (c->slots[idx] != NULL)      return 0;
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