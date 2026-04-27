#include "canal.h"
#include <string.h>

// Determina el paso en la dirección del movimiento.
// Si direccion == 0 (izquierda -> derecha), avanza +1.
// Si direccion == 1 (derecha -> izquierda), avanza -1.
static inline int dir_step(int direccion)
{
    return (direccion == 0) ? +1 : -1;
}

// Devuelve el índice de entrada del canal según la dirección.
// Para dirección 0 (izquierda), la entrada es el inicio (0).
// Para dirección 1 (derecha), la entrada es el final (largo-1).
static inline int entrada_idx(canal_t *c, int direccion)
{
    return (direccion == 0) ? 0 : (c->largo - 1);
}

// Setear parametros del canal con el config
void canal_init(canal_t *c, const config_t *cfg)
{
	c->largo = cfg->canal.largo;
	c->limite_w = cfg->canal.parametro_w;
	strncpy(c->metodo_flujo, cfg->canal.metodo_flujo, sizeof(c->metodo_flujo));

	// Direccion por default: izquierda
	c->direccion_actual = 0;
	
	c->ocupacion = 0;

	for (int i = 0; i < c->largo; i++) {
	    c->slots[i] = NULL;
	}
}

// Metodo para avanzar los barcos 
void canal_avanzar(canal_t *c)
{
    int dir = dir_step(c->direccion_actual);

    int start = (dir == +1) ? c->largo - 1 : 0;
    int end   = (dir == +1) ? -1 : c->largo;
    int step  = (dir == +1) ? -1 : +1;
	
	printf("[CANAL] Avanzando...\n");
	
    for (int i = start; i != end; i += step) {

        barco_t *b = c->slots[i];
        if (!b) continue;

        int nueva_pos = i + dir * b->velocidad;

        // salida del canal
        if (nueva_pos < 0 || nueva_pos >= c->largo) {
            printf("Barco %d salió\n", b->id);
            c->slots[i] = NULL;
            c->ocupacion--;
			printf("[CANAL] Barco %d SALIO del canal\n", b->id);
            continue;
        }

        c->slots[i] = NULL;
        c->slots[nueva_pos] = b;
        b->pos_canal = nueva_pos;
		
		printf("  Barco %d -> %d\n", b->id, nueva_pos);
    }
}

// Metodo para insertar nuevo barco al canal
void canal_insertar(canal_t *c, barco_t *b)
{
    if (b->direccion != c->direccion_actual)
        return;

    int entrada = entrada_idx(c, b->direccion);

    if (c->slots[entrada] == NULL) {
        c->slots[entrada] = b;
        b->pos_canal = entrada;
        c->ocupacion++;
		printf("[CANAL] Barco %d entra en %d\n", b->id, b->pos_canal);
    }
	
}

// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b)
{
    int dir = dir_step(b->direccion);
    int entrada = entrada_idx(c, b->direccion);

    // entrada libre
    if (c->slots[entrada] != NULL)
        return 0;

    // revisar rango de movimiento
    for (int i = 1; i <= b->velocidad; i++) {

        int idx = entrada + dir * i;

        if (idx < 0 || idx >= c->largo)
            break;

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