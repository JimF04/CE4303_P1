#include "canal.h"
#include "flow_policy.h"
#include <stdio.h>
#include <string.h>
#include "scheduler/scheduler.h"

int pasa_buque = 0;
extern scheduler_t *scheduler_global;


// Setear parametros del canal con el config
void canal_init(canal_t *c, const config_t *cfg)
{
    c->largo = cfg->canal.largo;

    c->direccion_actual = -1; // canal sin dirección al inicio
	
    c->ocupacion = 0;

	for (int i = 0; i < c->largo; i++) {
	    c->slots[i] = NULL;
	    c->slot_mutex[i] = xSemaphoreCreateMutex(); // uno por slot
	    configASSERT(c->slot_mutex[i] != NULL);
	}

	c->meta_mutex = xSemaphoreCreateMutex(); // para ocupacion y direccion_actual
	configASSERT(c->meta_mutex != NULL);
	
	c->policy = flow_policy_create(cfg->canal.metodo_flujo, cfg);
	c->policy->init(c->policy, c, cfg);

}


//recurso que usan los barcos para moverse
void canal_mover_barco(canal_t *c, barco_t *b)
{
    if (pasa_buque) return;
    if (b->pos_canal < 0) return;

    int pos_actual = b->pos_canal;
    int dir = c->direccion_actual;

    //  1. Calcular destino
    int destino = pos_actual;
    for (int p = 0; p < b->velocidad; p++) {
        int siguiente = (dir == 0) ? destino + 1 : destino - 1;
        if (siguiente < 0 || siguiente >= c->largo) break;
        if (c->slots[siguiente] != NULL) break;
        destino = siguiente;
    }

    if (destino == pos_actual) return; // nada que hacer

    // 2. Tomar los dos slots (menor índice primero)
    // Esto evita deadlock entre barcos moviéndose en sentidos opuestos
    int lock_1 = (pos_actual < destino) ? pos_actual : destino;
    int lock_2 = (pos_actual < destino) ? destino    : pos_actual;

    xSemaphoreTake(c->slot_mutex[lock_1], portMAX_DELAY);
    xSemaphoreTake(c->slot_mutex[lock_2], portMAX_DELAY);

    // 3. Re-verificar destino 
    if (c->slots[destino] == NULL) {
        c->slots[destino]    = b;
        c->slots[pos_actual] = NULL;
        b->pos_canal         = destino;
        b->posicion_guardada = destino;
    }

    xSemaphoreGive(c->slot_mutex[lock_2]);
    xSemaphoreGive(c->slot_mutex[lock_1]);

    // 4. Verificar si llegó al borde
    int borde = (dir == 0) ? c->largo - 1 : 0;
    if (b->pos_canal == borde) {
        xSemaphoreTake(c->slot_mutex[borde], portMAX_DELAY);

        if (c->slots[borde] == b) {
            c->slots[borde] = NULL;
            b->state             = DONE;
            b->pos_canal         = -1;
            b->posicion_guardada = -1;

            // meta_mutex para tocar ocupacion y direccion_actual
            xSemaphoreTake(c->meta_mutex, portMAX_DELAY);
            c->ocupacion--;
            if (c->ocupacion == 0)
                c->direccion_actual = -1;
            xSemaphoreGive(c->meta_mutex);

            if (c->policy && c->policy->notify_salio)
                c->policy->notify_salio(c->policy, b->direccion);

            printf("[CANAL] Barco %d salió del canal\n", b->id);
        }

        xSemaphoreGive(c->slot_mutex[borde]);
    }
}

int canal_insertar(canal_t *c, barco_t *b)
{
    xSemaphoreTake(c->meta_mutex, portMAX_DELAY);

    if (!canal_puede_entrar(c, b)) {
        xSemaphoreGive(c->meta_mutex);
        return 0;
    }

    int pos = (b->posicion_guardada >= 0 &&
               b->posicion_guardada < c->largo &&
               c->slots[b->posicion_guardada] == NULL)
              ? b->posicion_guardada
              : (b->direccion == 0 ? 0 : c->largo - 1);

    if (c->ocupacion == 0)
        c->direccion_actual = b->direccion;

    // Tomar el slot de entrada antes de escribir
    xSemaphoreTake(c->slot_mutex[pos], portMAX_DELAY);
    c->slots[pos]        = b;
    b->posicion_guardada = pos;
    b->pos_canal         = pos;
    c->ocupacion++;
    b->state             = RUNNING;
    xSemaphoreGive(c->slot_mutex[pos]);

    printf("[CANAL] Barco %d entró al canal (pos=%d)\n", b->id, pos);

    xSemaphoreGive(c->meta_mutex);
    return 1;
}

// Retorna la distancia mínima que debe haber entre la entrada
// y el barco más cercano a ella, para que el nuevo no lo alcance.
int canal_entrada_segura(canal_t *c, barco_t *nuevo)
{
    int entrada = (nuevo->direccion == 0) ? 0 : c->largo - 1;
    int paso    = (nuevo->direccion == 0) ? 1 : -1;

    // Buscar el barco más cercano a la entrada
    for (int i = entrada; i >= 0 && i < c->largo; i += paso) {
        if (c->slots[i] == NULL) continue;

        barco_t *delante = c->slots[i];
        int d = abs(i - entrada); // distancia actual entre entrada y delante

        // Si delante es igual o más rápido: nunca lo alcanza
        if (delante->velocidad >= nuevo->velocidad)
            return 1;

        // delante es más lento: calcular si el nuevo lo alcanza antes
        // de que delante salga del canal.
        //
        // Slots que le faltan a delante para salir:
        int slots_restantes_delante = (nuevo->direccion == 0)
            ? (c->largo - 1 - i)   // dir IZQ: le falta llegar al final
            : i;                    // dir DER: le falta llegar al 0

        // Ticks que tarda delante en salir:
        // sale cuando acumula 'slots_restantes_delante' avances
        // redondeando hacia arriba
        int ticks_para_salir = (slots_restantes_delante + delante->velocidad - 1)
                               / delante->velocidad;

        // Posición del nuevo en ese tick (si entrara ahora):
        int pos_nuevo_al_salir = nuevo->velocidad * ticks_para_salir;

        // Posición de delante en ese tick (ya fuera del canal):
        // Para verificar, nos basta con que en cada tick intermedio
        // el nuevo no lo alcance. La condición simplificada:
        // el nuevo nunca supera a delante si:
        // vel_nuevo * t < d + vel_delante * t  para t = 1..ticks_para_salir
        // el peor caso es t=1 (primer tick):
        int pos_nuevo_t1  = nuevo->velocidad;      // desde pos 0
        int pos_delante_t1 = d + delante->velocidad; // desde pos d

        if (pos_nuevo_t1 >= pos_delante_t1) {
            // Choca en el primer tick
            return 0;
        }

        // Verificar tick a tick hasta que delante salga
        int pos_n = 0;
        int pos_d = d;
        for (int t = 1; t <= ticks_para_salir; t++) {
            pos_n += nuevo->velocidad;
            pos_d += delante->velocidad;

            // Si delante ya salió, el nuevo tiene vía libre
            if (nuevo->direccion == 0 && pos_d >= c->largo) break;
            if (nuevo->direccion == 1 && pos_d < 0)         break;

            if (pos_n >= pos_d) return 0; // choque
        }

        return 1; // seguro
    }

    return 1; // canal vacío
}

// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b)
{
    if (!b) return 0;

    if (c->policy && !c->policy->allow(c->policy, c, b))
        return 0;

    int entrada_real = (b->direccion == 0 ? 0 : c->largo - 1);

    if (c->ocupacion == 0) {
        if (b->posicion_guardada >= 0 &&
            b->posicion_guardada < c->largo &&
            c->slots[b->posicion_guardada] == NULL)
            return 1;

        return (c->slots[entrada_real] == NULL);
    }

    if (c->direccion_actual != b->direccion){
		return 0;
	}

    if (c->slots[entrada_real] != NULL){
        return 0;
	}
	
	if (!canal_entrada_segura(c, b)){
	    return 0;
	}

    return 1;
}

void canal_remover_barco(canal_t *c, barco_t *b)
{
    
    xSemaphoreTake(c->meta_mutex, portMAX_DELAY);

    if (!b) {
        xSemaphoreGive(c->meta_mutex);
        return;
    }

    if (b->pos_canal == -1) {
        xSemaphoreGive(c->meta_mutex);
        return;
    }

    int pos = b->pos_canal; //posicion de barco en el canal

    if (pos >= 0 && pos < c->largo && c->slots[pos] == b) {
	    c->slots[pos] = NULL; //se quita el barco del canal
	    c->ocupacion--; //se va la ocupacion
	}
	
	if (c->ocupacion == 0)
	    c->direccion_actual = -1;
	
	b->posicion_guardada = pos; //se guarda la posicion para restaurar el estado luego
	b->pos_canal = -1; //se pone como que no esta, para que no se dibuje
    b->state = READY;

    printf("[CANAL] Barco %d removido\n", b->id);
    xSemaphoreGive(c->meta_mutex); // UNLOCK

}



void canal_viene_buque(canal_t *c){

    pasa_buque = 1;

    xSemaphoreTake(c->meta_mutex, portMAX_DELAY); // LOCK

    for (int i = 0; i < c->largo; i++) {
            if (c->slots[i] == NULL) continue;

            barco_t *b = c->slots[i];
            b->posicion_guardada = b->pos_canal; //se guarda la posicion para restaurar el estado luego
	        b->pos_canal = -1; //se pone como que no esta, para que no se dibuje
            b->state = READY;
            scheduler_global->enqueue(b);
          
            
            c->slots[i] = NULL; //se quita el barco del canal
	        c->ocupacion--; //se va la ocupacion


    }


    xSemaphoreGive(c->meta_mutex); // UNL

    xTaskCreate(
    buque_task,     // función
    "buque_task",   // nombre
    4096,           // stack
    NULL,           // parámetro
    5,              // prioridad
    NULL            // handle (opcional)

);

}

void buque_task(void *arg){

    const int col = 10;     // columna fija
    const int inicio = 10;
    const int fin = 35;

    int prev = -1;

    printf("\033[?25l"); // ocultar cursor 

    for (int i = inicio; i <= fin; i++) {

        // borrar posición anterior
        if (prev != -1) {
            printf("\033[%d;%dH        ", prev, col);
        }

        // dibujar buque nuevo
        printf("\033[%d;%dH", i, col);
        printf("[######]");
        fflush(stdout);

        prev = i;

        vTaskDelay(pdMS_TO_TICKS(120));
    }

    // limpiar última posición
    printf("\033[%d;%dH       ", prev, col);

    printf("\033[?25h"); // mostrar cursor otra vez

    pasa_buque = 0;

    vTaskDelete(NULL);
}


void canal_print(canal_t *c)
{

    if(pasa_buque) return;
    xSemaphoreTake(c->meta_mutex, portMAX_DELAY); // LOCK


    printf("\n========== TICK ==========\n");

    printf("[");

    for (int i = 0; i < c->largo; i++) {
        if (c->slots[i] == NULL)
            printf(".");
        else
            printf("%d", c->slots[i]->id);
    }

    printf("]\n");
    xSemaphoreGive(c->meta_mutex); // UNLOCK




}

