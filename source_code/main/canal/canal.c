#include "canal.h"
#include "flow_policy.h"
#include <stdio.h>
#include <string.h>

// Setear parametros del canal con el config
void canal_init(canal_t *c, const config_t *cfg)
{
    c->largo = cfg->canal.largo;

    c->direccion_actual = -1; // canal sin dirección al inicio
	
    c->ocupacion = 0;
	
    c->barcos_pasados = 0;

    for (int i = 0; i < c->largo; i++)
        c->slots[i] = NULL;

    c->policy = flow_policy_create(cfg->canal.metodo_flujo, cfg);
    c->policy->init(c->policy, c, cfg);
}


//recurso que usan los barcos para moverse
void canal_mover_barco(canal_t *c, barco_t *b)
{
    if (c->direccion_actual == 0) {
        // izquierda -> derecha: iterar de derecha a izquierda
        for (int i = c->largo - 2; i >= 0; i--) {
            if (c->slots[i] == NULL) continue;

            barco_t *barco = c->slots[i];
            int pasos = barco->velocidad;  // cuántos slots avanza este tick
            int nueva_pos = i;

            // Intentar avanzar 'pasos' slots
            for (int p = 0; p < pasos; p++) {
                int siguiente = nueva_pos + 1;
                if (siguiente >= c->largo) break;        // llegó al borde
                if (c->slots[siguiente] != NULL) break;  // bloqueado por otro barco
                nueva_pos = siguiente;
            }

            if (nueva_pos != i) {
                c->slots[nueva_pos] = barco;
                c->slots[i] = NULL;
                barco->posicion_guardada = nueva_pos;
                barco->pos_canal = nueva_pos;
            }
        }
    } else {
        // derecha -> izquierda: iterar de izquierda a derecha
        for (int i = 1; i < c->largo; i++) {
            if (c->slots[i] == NULL) continue;

            barco_t *barco = c->slots[i];
            int pasos = barco->velocidad;
            int nueva_pos = i;

            for (int p = 0; p < pasos; p++) {
                int siguiente = nueva_pos - 1;
                if (siguiente < 0) break;                // llegó al borde
                if (c->slots[siguiente] != NULL) break;  // bloqueado
                nueva_pos = siguiente;
            }

            if (nueva_pos != i) {
                c->slots[nueva_pos] = barco;
                c->slots[i] = NULL;
                barco->posicion_guardada = nueva_pos;
                barco->pos_canal = nueva_pos;
            }
        }
    }

    // Sacar barcos que llegaron al borde
    int borde = (c->direccion_actual == 0 ? c->largo - 1 : 0);

    if (c->slots[borde] != NULL) {
        barco_t *saliente = c->slots[borde];
        c->slots[borde] = NULL;
        c->ocupacion--;

        saliente->state = DONE;
        saliente->posicion_guardada = -1;
        saliente->pos_canal = -1;

        if (c->policy && c->policy->notify_salio)
            c->policy->notify_salio(c->policy, saliente->direccion);

        printf("[CANAL] Barco %d salió del canal\n", saliente->id);
    }

    if (c->ocupacion == 0)
        c->direccion_actual = -1;
}

int canal_insertar(canal_t *c, barco_t *b)
{
    if (!canal_puede_entrar(c, b))
        return 0;

    int pos;

    // Determinar posición final (guardada o entrada real)
    if (b->posicion_guardada >= 0 &&
        b->posicion_guardada < c->largo &&
        c->slots[b->posicion_guardada] == NULL)
    {
        pos = b->posicion_guardada;
    }
    else
    {
        pos = (b->direccion == 0 ? 0 : c->largo - 1);
    }

    // Si canal estaba vacío -> fijar nueva dirección
    if (c->ocupacion == 0)
        c->direccion_actual = b->direccion;

    // Insertar
    c->slots[pos] = b;
    b->posicion_guardada = pos;
	b->pos_canal = pos;
    c->ocupacion++;
    b->state = RUNNING;

    printf("[CANAL] Barco %d entró al canal (pos=%d, RUNNING)\n", b->id, pos);

    return 1;
}


// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b)
{
    if (!b) return 0;

    // Política
    if (c->policy && !c->policy->allow(c->policy, c, b))
        return 0;

    int entrada_real = (b->direccion == 0 ? 0 : c->largo - 1);

    // Canal vacío
    if (c->ocupacion == 0) {
        if (b->posicion_guardada >= 0 &&
            b->posicion_guardada < c->largo &&
            c->slots[b->posicion_guardada] == NULL)
            return 1;

        return (c->slots[entrada_real] == NULL);
    }

    // Canal NO vacío -> dirección debe coincidir
    if (c->direccion_actual != b->direccion)
        return 0;

    // Entrada real debe estar libre
    if (c->slots[entrada_real] != NULL)
        return 0;

    return 1;
}



void canal_remover_barco(canal_t *c, barco_t *b)
{
    if (!b) return; //que el barco sea valido

    if (b->pos_canal == -1) return; //si no esta en el canal, nada que remover

    int pos = b->pos_canal; //posicion de barco en el canal

    if (pos >= 0 && pos < c->largo && c->slots[pos] == b) {
	    c->slots[pos] = NULL; //se quita el barco del canal
	    c->ocupacion--; //se va la ocupacion
	}
	
	if (c->ocupacion == 0)
	    c->direccion_actual = -1;
	
	b->posicion_guardada = pos; //se guarda la posicion para restaurar el estado luego
	b->pos_canal = -1; //se pone como que no esta, para que no se dibuje

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