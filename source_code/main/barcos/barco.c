#include <stdio.h>
#include <string.h>
#include "barco.h"
#include <math.h>
#include <canal/canal.h>

#include <stdbool.h>

#include "scheduler/scheduler.h"

#include "../debug.h"

extern canal_t *canal_global;
extern scheduler_t sched;


static barco_t barcos[8]; //lista de barcos
static int barcos_total = 0; //cuantos barcos existen
static int id_global = 0; //id para los barcos

// ========================
//   TAREA DE CADA BARCO
// ========================
void barco_task(void *arg)
{
    barco_t *b = (barco_t *)arg;

    while (1) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //notificacion para que se despierte


        //solo mueve
        if (b->pos_canal >= 0) {
            canal_mover_barco(canal_global, b); //usamos el recurso para moverse
        }
		
		if (b->state != DONE)
		    b->state = READY;
		
		//print_tasks_real();

        if (b->state == DONE) { //si el barco termina:
            sched.notify_done(b); //se notifica que ya termino 
            vTaskDelete(NULL); //se elimina ese task
        }
    }
}
// ========================
//   CREACIÓN DE BARCOS
// ========================

// Metodo para crear un barco (proceso)
bool crear_barco(const config_t *cfg, const char tipo_b[16], int direccion_b)
{
    int capacidad_logica = cfg->barcos.cantidad * 2; 


    if (capacidad_logica > BARCOS_MAX) {
        printf("ERROR: config.ini solicita %d barcos (máximo permitido = %d)\n",
               capacidad_logica, BARCOS_MAX);
        return false;
    }

    if (barcos_total >= capacidad_logica) {
        printf("Límite del config alcanzado (%d barcos)\n", capacidad_logica);
        return false;
    }

    if (barcos_total >= BARCOS_MAX) {
        printf("Límite físico alcanzado (%d barcos)\n", BARCOS_MAX);
        return false;
    }

    barco_t *b = &barcos[barcos_total]; //se guardan los barcos que hay    

    // aqui se le dan las propiedades al barco

    b->id = id_global; //se le da el id

    b->en_cola = 0; //se pone que esta en cola

    strncpy(b->tipo, tipo_b, sizeof(b->tipo)); //[PES,NOR,PAT] tipos de barco
    b->tipo[15] = '\0';
    b->direccion = direccion_b; //izquierda o derecha
    b->pos_canal = -1; //-1 = no tiene posicion en el canal
    b->posicion_guardada = -1;  // -1 =  aun no tiene una posicion guardada
    b->state = READY;  //se pone que esta ready para ejecutarse

    asignar_velocidad(cfg, b); //se le asigna la velocidad segun el tipo de barco
    asignar_prioridad(cfg, b); //se le asigna la prioridad segun el tipo de barco
    asignar_deadline(cfg, b); //se le asigna el deadline segun el tipo de barco


    char dir = (direccion_b == 0) ? 'L' : 'R';
    snprintf(b->nombre, sizeof(b->nombre), "%s_%c_%d", tipo_b, dir, b->id);

    xTaskCreate(barco_task,b->nombre,4096,b,5,&b->handle); //se crea el task, este se empieza a ejecutar automaticamente

    id_global++;
    barcos_total++;

    printf("Barco %d creado (%s)\n", b->id, b->tipo);

    return true; 
}

// Metodo para asignar la velocidad a los barcos
void asignar_velocidad(const config_t *cfg, barco_t *b)
{
    int base = cfg->barcos.velocidad_base;

    if (strcmp(b->tipo, "NOR") == 0) {
        b->velocidad = base;
    } 
    else if (strcmp(b->tipo, "PES") == 0) {
        b->velocidad = base + 1;
    } 
    else if (strcmp(b->tipo, "PAT") == 0) {
        b->velocidad = base + 2;
    } 
    else {
        b->velocidad = base; // fallback
    }
}


// Metodo para asignar la velocidad a los barcos
void asignar_prioridad(const config_t *cfg, barco_t *b)
{
    int base = cfg->barcos.prioridad_base;

    if (strcmp(b->tipo, "NOR") == 0) {
        b->prioridad = base + 2;
    } 
    else if (strcmp(b->tipo, "PES") == 0) {
        b->prioridad = base ;
    } 
    else if (strcmp(b->tipo, "PAT") == 0) {
        b->prioridad = base + 1;
    } 
    else {
        b->prioridad = base; // fallback
    }
}



// Metodo para asignar el deadline a los barcos
void asignar_deadline(const config_t *cfg, barco_t *b)
{
    int base = cfg->barcos.prioridad_base;

    if (strcmp(b->tipo, "NOR") == 0) {
        b->deadLine = base + 2;
    } 
    else if (strcmp(b->tipo "PES") == 0) {
        b->deadLine = base + 1;
    } 
    else if (strcmp(b->tipo, "PAT") == 0) {
        b->deadLine = base;
    } 
    else {
        b->deadLine = base; // fallback
    }
}







// Metodo para crear barcos por default (depende del config)
void barcos_init(const config_t *cfg)
{
    id_global    = 0;
    barcos_total = 0;

    int n_izq = cfg->barcos.cantidad_izquierda;
    int n_der = cfg->barcos.cantidad_derecha;

    // Validar que no supere cantidad declarada
    if (n_izq > cfg->barcos.cantidad) {
        printf("[WARN] izquierda tiene %d tipos pero cantidad=%d, usando %d\n",
               n_izq, cfg->barcos.cantidad, cfg->barcos.cantidad);
        n_izq = cfg->barcos.cantidad;
    }

    if (n_der > cfg->barcos.cantidad) {
        printf("[WARN] derecha tiene %d tipos pero cantidad=%d, usando %d\n",
               n_der, cfg->barcos.cantidad, cfg->barcos.cantidad);
        n_der = cfg->barcos.cantidad;
    }

    // IZQUIERDA — solo los que están definidos
    for (int i = 0; i < n_izq; i++) {
        if (strlen(cfg->barcos.izquierda[i]) == 0) break;
        crear_barco(cfg, cfg->barcos.izquierda[i], 0);
    }

    // DERECHA — solo los que están definidos
    for (int i = 0; i < n_der; i++) {
        if (strlen(cfg->barcos.derecha[i]) == 0) break;
        crear_barco(cfg, cfg->barcos.derecha[i], 1);
    }

    printf("[BARCOS] Total creados: %d (izq=%d, der=%d)\n",
           barcos_total, n_izq, n_der);
}

// ========================
//   ELIMINACION DE BARCOS
// ========================
void eliminar_barco(int index)
{
    if (index < 0 || index >= barcos_total) return; //se elimina con el indice en la lista

    barco_t *b = &barcos[index]; //se toma el barco

    b->handle = NULL; 
    b->state = DONE; //se pone como listo
    b->pos_canal = -1; //se quita del canal

    //MARCAR COMO LIBRE
    b->id = -1;
    strcpy(b->nombre, "FREE");
    

    printf("Barco eliminado (slot %d liberado)\n", index);
}






// ========================
//   ACCESORES
// ========================

barco_t* barcos_get(int index)
{
    if (index < 0 || index >= barcos_total) {
        return NULL;
    }
    return &barcos[index];
}

int barcos_count()
{
    return barcos_total;
}
