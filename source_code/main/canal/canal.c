#include "canal.h"
#include "flow_policy.h"
#include <stdio.h>
#include <string.h>
#include "scheduler/scheduler.h"


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

	c->pasa_buque = 0;
}


int canal_lleno(canal_t *c)
{
    if (!c) return 1;

    xSemaphoreTake(c->meta_mutex, portMAX_DELAY);

    int lleno = (c->ocupacion >= c->largo);

    xSemaphoreGive(c->meta_mutex);

    return lleno;
}

barco_t *canal_barco_min(canal_t *c, int criterio)
{
    if (!c) return NULL;

    xSemaphoreTake(c->meta_mutex, portMAX_DELAY);

    barco_t *min = NULL;
    int min_val = 0;

    for (int i = 0; i < c->largo; i++) {
        barco_t *b = c->slots[i];
        if (!b) continue;

        int val = 0;

        switch (criterio) {
            case 0:
                val = b->velocidad;
                break;

            case 1:
                val = b->prioridad;
                break;

            case 2:
                val = b->id;
                break;

            default:
                val = b->velocidad;
                break;
        }

        if (min == NULL || val < min_val) {
            min = b;
            min_val = val;
        }
    }

    xSemaphoreGive(c->meta_mutex);

    return min;
}


barco_t *canal_barco_max(canal_t *c, int criterio)
{
    if (!c) return NULL;

    xSemaphoreTake(c->meta_mutex, portMAX_DELAY);

    barco_t *max = NULL;
    int max_val = 0;

    for (int i = 0; i < c->largo; i++) {
        barco_t *b = c->slots[i];
        if (!b) continue;

        int val = 0;

        switch (criterio) {
            case 0:
                val = b->velocidad;
                break;

            case 1:
                val = b->deadline;
                break;

            case 2:
                val = b->id;
                break;

            default:
                val = b->velocidad;
                break;
        }

        if (max == NULL || val > max_val) {
            max = b;
            max_val = val;
        }
    }

    xSemaphoreGive(c->meta_mutex);

    return max;
}

//recurso que usan los barcos para moverse
void canal_mover_barco(canal_t *c, barco_t *b)
{
    if (c->pasa_buque || b->pos_canal < 0) return;

    int pos_actual = b->pos_canal;
    int dir = c->direccion_actual;
    int paso = (dir == 0) ? 1 : -1;
    
    // 1. Intentar calcular si puede avanzar su VELOCIDAD completa
    int puede_moverse_completo = 1;
    int destino_final = pos_actual + (b->velocidad * paso);

    for (int p = 1; p <= b->velocidad; p++) {
        int revisar = pos_actual + (p * paso);
        
        // Si el siguiente slot es la salida del canal, el camino está libre
        if (revisar < 0 || revisar >= c->largo) break; 

        if (c->slots[revisar] != NULL) {
            puede_moverse_completo = 0; // Hay un obstáculo
            break;
        }
    }

    // Si no puede completar su movimiento de 3 (o su velocidad), se queda quieto
    if (!puede_moverse_completo) return;

    // Calcular destino real (limitado por los bordes del canal)
    int destino = pos_actual + (b->velocidad * paso);
    if (destino < 0) destino = 0;
    if (destino >= c->largo) destino = c->largo - 1;

    // 2. Bloqueo de slots y movimiento (Tu lógica de mutex actual es correcta)
    int lock_1 = (pos_actual < destino) ? pos_actual : destino;
    int lock_2 = (pos_actual < destino) ? destino    : pos_actual;

    xSemaphoreTake(c->slot_mutex[lock_1], portMAX_DELAY);
    if (lock_1 != lock_2) xSemaphoreTake(c->slot_mutex[lock_2], portMAX_DELAY);

    if (c->slots[destino] == NULL) {
        c->slots[destino]    = b;
        c->slots[pos_actual] = NULL;
        b->pos_canal         = destino;
        b->posicion_guardada = destino;
    }

    if (lock_1 != lock_2) xSemaphoreGive(c->slot_mutex[lock_2]);
    xSemaphoreGive(c->slot_mutex[lock_1]);

    // 3. Re-verificar destino 
//    if (c->slots[destino] == NULL) {
//        c->slots[destino]    = b;
//        c->slots[pos_actual] = NULL;
//        b->pos_canal         = destino;
//        b->posicion_guardada = destino;
//    }
//
//    xSemaphoreGive(c->slot_mutex[lock_2]);
//    xSemaphoreGive(c->slot_mutex[lock_1]);

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

//            if (c->policy && c->policy->notify_salio)
//                c->policy->notify_salio(c->policy, b->direccion);

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
	
	if (c->policy && c->policy->notify_salio)
	    c->policy->notify_salio(c->policy, b->direccion);

    printf("[CANAL] Barco %d entró al canal (pos=%d)\n", b->id, pos);

    xSemaphoreGive(c->meta_mutex);
    return 1;
}


// Metodo para verificar si puede entrar al canal
int canal_puede_entrar(canal_t *c, barco_t *b)
{
    if (!b) return 0;
	
	if(c->pasa_buque) return 0;

    // Respetar la política de flujo 
    if (c->policy && !c->policy->allow(c->policy, c, b))
        return 0;

    // Si el canal tiene barcos, deben ir en la misma dirección
    if (c->ocupacion > 0 && c->direccion_actual != b->direccion) {
        return 0;
    }

    int entrada_real = (b->direccion == 0 ? 0 : c->largo - 1);
    int paso = (b->direccion == 0 ? 1 : -1);

    // Verificar si los slots necesarios para su velocidad están libres
    // Si es barco velocidad 3, verifica slots 0, 1 y 2.
    for (int i = 0; i < b->velocidad; i++) {
        int check_pos = entrada_real + (i * paso);
        
        // Si la posición se sale del canal, significa que el canal es muy corto, 
        // pero el espacio físico está "libre" más allá.
        if (check_pos < 0 || check_pos >= c->largo) break;

        if (c->slots[check_pos] != NULL) {
            return 0; // Hay alguien estorbando la entrada
        }
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

    c->pasa_buque = 1;

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
    (void *)c,           // parámetro
    5,              // prioridad
    NULL            // handle (opcional)

);

}

void buque_task(void *pvParameters) {
	
	canal_t *c = (canal_t *) pvParameters;

    const int col = 10;     // columna fija
    const int inicio = 0;
    const int fin = 100;

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

    c->pasa_buque = 0;

    vTaskDelete(NULL);
}


void canal_print(canal_t *c)
{

    if(c->pasa_buque) return;
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