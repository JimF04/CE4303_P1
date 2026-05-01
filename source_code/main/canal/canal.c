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
    if (!b || b->pos_canal < 0) return;

    int actual = b->pos_canal;
    int dir = (b->direccion == 0) ? +1 : -1;

    // 🔥 mover paso a paso (NO saltar)
    for (int i = 0; i < b->velocidad; i++) {

        int siguiente = actual + dir;

        // 🔥 salida del canal
        if (siguiente < 0 || siguiente >= c->largo) {

            c->slots[actual] = NULL;

            b->posicion_guardada = actual;  // 💾 guardar progreso
            b->pos_canal = -1;
            b->state = DONE;

            c->ocupacion--;

            printf("[BARCO] %d salió del canal\n", b->id);
            return;
        }

        // 🔥 colisión → se detiene
        if (c->slots[siguiente] != NULL) {
            return;
        }

        // 🔥 mover un paso
        c->slots[actual] = NULL;
        c->slots[siguiente] = b;

        actual = siguiente;
        b->pos_canal = actual;
    }
}

void canal_insertar(canal_t *c, barco_t *b)
{
    if (!b) return;

    // 🔥 Canal ocupado → no entra nadie más
    if (c->ocupacion > 0) {
        return;
    }

    // 🔥 Ya está en el canal
    if (b->pos_canal != -1) {
        return;
    }

    // 🔥 Estado válido
    if (b->state != RUNNING && b->state != READY) return;

    // 🔥 Política
    if (b->direccion == 0 && !c->policy->allow_left(c->policy, c))  return;
    if (b->direccion == 1 && !c->policy->allow_right(c->policy, c)) return;

    int entrada = -1;

    // 🔥 1. Intentar usar posición guardada (si es válida)
    if (b->posicion_guardada >= 0 &&
        b->posicion_guardada < c->largo &&
        c->slots[b->posicion_guardada] == NULL)
    {
        entrada = b->posicion_guardada;
    }
    else {
        // 🔥 2. Entrada REAL según dirección (FIX PRINCIPAL)
        if (b->direccion == 0)
            entrada = 0;                // izquierda entra por 0
        else
            entrada = c->largo - 1;     // derecha entra por el final
    }

    if (entrada < 0 || entrada >= c->largo) return;

    // 🔥 Validar que el slot esté libre
    if (!canal_puede_entrar(c, b, entrada)) {
        return;
    }

    // 🔥 Insertar
    c->slots[entrada] = b;
    b->pos_canal = entrada;
    c->ocupacion++;

    printf("[CANAL] Barco %d (%s) insertado en slot %d\n",
           b->id, b->nombre, entrada);
}


// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b, int entrada)
{
    if (!b) return 0;

    // 🔥 Canal ocupado (doble seguridad)
    if (c->ocupacion > 0) return 0;

    if (entrada < 0 || entrada >= c->largo) return 0;

    // 🔥 Solo validar el slot de entrada
    if (c->slots[entrada] != NULL) return 0;

    return 1;
}



void canal_remover_barco(canal_t *c, barco_t *b)
{
    if (!b) return;

    if (b->pos_canal == -1) return;

    int pos = b->pos_canal;

    if (pos >= 0 && pos < c->largo && c->slots[pos] == b) {
    c->slots[pos] = NULL;
    c->ocupacion--;
}

b->posicion_guardada = pos;
b->pos_canal = -1;

    printf("[CANAL] Barco %d removido\n", b->id);
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