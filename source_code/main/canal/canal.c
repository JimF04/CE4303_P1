#include "canal.h"
#include <string.h>

void canal_init(canal_t *c, const config_t *cfg)
{
	c->largo = cfg->canal.largo;
	c->limite_w = cfg->canal.parametro_w;
	strncpy(c->metodo_flujo, cfg->canal.metodo_flujo, sizeof(c->metodo_flujo));

	// Direccion por default: izquierda
	c->direccion_actual = 0;
}

void canal_avanzar(canal_t *c)
{
    // salida según dirección 
    if (c->direccion_actual == 0) {
        // izq -> der
        if (c->slots[c->largo - 1] != NULL) {
            printf("Barco %d salió del canal\n",
                   c->slots[c->largo - 1]->id);
            c->slots[c->largo - 1] = NULL;
            c->ocupacion--;
        }

        // mover todos hacia la derecha
        for (int i = c->largo - 1; i > 0; i--) {
            c->slots[i] = c->slots[i - 1];
        }

        c->slots[0] = NULL;
    }

    else {
        // der -> izq
        if (c->slots[0] != NULL) {
            printf("Barco %d salió del canal\n",
                   c->slots[0]->id);
            c->slots[0] = NULL;
            c->ocupacion--;
        }

        // mover todos hacia la izquierda
        for (int i = 0; i < c->largo - 1; i++) {
            c->slots[i] = c->slots[i + 1];
        }

        c->slots[c->largo - 1] = NULL;
    }
}

void canal_insertar(canal_t *c, barco_t *b)
{
    // izq -> der
    if ((c->direccion_actual == 0) && (b->direccion == 0)) {

        if (c->slots[0] == NULL) {
            c->slots[0] = b;
            c->ocupacion++;
            printf("Barco %d entra al canal por IZQUIERDA\n", b->id);
        }
    }

    // der -> izq
    else if ((c->direccion_actual == 1) && (b->direccion == 1)) {

        if (c->slots[c->largo - 1] == NULL) {
            c->slots[c->largo - 1] = b;
            c->ocupacion++;
            printf("Barco %d entra al canal por DERECHA\n", b->id);
        }
    }
}