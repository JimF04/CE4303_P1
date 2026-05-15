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



static barco_t barcos[BARCOS_MAX]; //lista de barcos

// Cantidad de slots usados (incluyendo slots FREE); nunca decrece
static int barcos_total = 0; 

// Cantidad de barcos vivos actualmente (decrece al eliminar)
static int cantidad = 0;

// ID global incremental, cada barco recibe un ID único para siempre
static int id_global = 0;

// ========================
//   TAREA DE CADA BARCO
// ========================
void barco_task(void *arg)
{
    barco_t *b = (barco_t *)arg;

    while (1) {
		
		// Espera hasta que el loop principal le dé una notificación 
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        // Si el barco está en el canal, avanza su posición
        if (b->pos_canal >= 0) {
            canal_mover_barco(canal_global, b); //usamos el recurso para moverse
        }
		
		// Si no terminó, vuelve a READY para el siguiente tick
		if (b->state != DONE)
		    b->state = READY;
		
		//si el barco termina:
        if (b->state == DONE) { 
            sched.notify_done(b); //se notifica que ya termino 
            vTaskDelete(NULL); //se elimina ese task
        }
    }
}
// ========================
//   CREACIÓN DE BARCOS
// ========================

// Metodo para crear un barco (proceso)
barco_t* crear_barco(const config_t *cfg, const char tipo_b[16], int direccion_b)
{
	
	// Verificar límite de barcos vivos simultáneos
    if (cantidad >= BARCOS_MAX) {
        printf("Límite físico alcanzado (%d barcos)\n", BARCOS_MAX);
        return NULL;
    }

	// Buscar un slot liberado por un barco anterior (id == -1 significa FREE)
	int slot = -1;
	for (int i = 0; i < barcos_total; i++) {
	    if (barcos[i].id == -1) {
	        slot = i;
	        break;
	    }
	}

	// Si no hay slot libre, expandir el pool al siguiente índice disponible
	if (slot == -1) {
	    if (barcos_total >= BARCOS_MAX) return NULL;
	    slot = barcos_total;
	    barcos_total++; 
	}

	barco_t *b = &barcos[slot];  

    // aqui se le dan las propiedades al barco

    b->id = id_global; //se le da el id

    b->en_cola = 0; //se pone que esta en cola

    strncpy(b->tipo, tipo_b, sizeof(b->tipo)); //[PES,NOR,PAT] tipos de barco
    b->tipo[15] = '\0';
    b->direccion = direccion_b; //izquierda o derecha
    b->pos_canal = -1; //-1 = no tiene posicion en el canal
    b->posicion_guardada = -1;  // -1 =  aun no tiene una posicion guardada
    b->state = READY;  //se pone que esta ready para ejecutarse

	// Asignar atributos según tipo (NOR, PES, PAT)
    asignar_velocidad(cfg, b); 
    asignar_prioridad(cfg, b); 
    asignar_deadline(cfg, b); 

	// Nombre único para identificación y debug
    char dir = (direccion_b == 0) ? 'L' : 'R';
    snprintf(b->nombre, sizeof(b->nombre), "%s_%c_%d", tipo_b, dir, b->id);

	// Lanzar el task de FreeRTOS, el puntero b es el argumento
    xTaskCreate(barco_task,b->nombre, 4096, b, 5, &b->handle); //se crea el task, este se empieza a ejecutar automaticamente

    id_global++;

    cantidad ++;

	printf("Barco %d creado (%s) [slot=%d, total=%d]\n",
	       b->id, b->tipo, slot, barcos_total);

    return b;
}

// ========================
//   ASIGNACIÓN DE ATRIBUTOS
// ========================

// Metodo para asignar la velocidad a los barcos
// Velocidad: PAT > PES > NOR
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

// Metodo para asignar la prioridad a los barcos
// Prioridad de scheduler: NOR > PAT > PES
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
// Deadline: PAT > PES > NOR
void asignar_deadline(const config_t *cfg, barco_t *b)
{
    int base = cfg->barcos.prioridad_base;

    if (strcmp(b->tipo, "NOR") == 0) {
        b->deadline = base ;
    } 
    else if (strcmp(b->tipo, "PES") == 0) {
        b->deadline = base + 1;
    } 
    else if (strcmp(b->tipo, "PAT") == 0) {
        b->deadline = base + 2 ;
    } 
    else {
        b->deadline = base; // fallback
    }
}

// ========================
//   INICIALIZACIÓN
// ========================


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
		barco_t *b = crear_barco(cfg, cfg->barcos.izquierda[i], 0);
		if (b) sched.enqueue(b);
    }

    // DERECHA — solo los que están definidos
    for (int i = 0; i < n_der; i++) {
        if (strlen(cfg->barcos.derecha[i]) == 0) break;
		barco_t *b = crear_barco(cfg, cfg->barcos.derecha[i], 1);
		if (b) sched.enqueue(b);
    }

    printf("[BARCOS] Total creados: %d (izq=%d, der=%d)\n",
           barcos_total, n_izq, n_der);
}

// ========================
//   ELIMINACION DE BARCOS
// ========================

void eliminar_barco(int index)
{
    if (index < 0 || index >= barcos_total) return;

    barco_t *b = &barcos[index];
    b->handle  = NULL; // el task ya se autodestruyó con vTaskDelete
    b->state   = DONE;
    b->pos_canal = -1;
    b->id      = -1;  // marca el slot como reutilizable
    strcpy(b->nombre, "FREE");
    cantidad--; // un barco menos vivo

}


// ========================
//   ACCESORES
// ========================

// Retorna el barco en el índice dado, o NULL si está fuera de rango.
barco_t* barcos_get(int index)
{
    if (index < 0 || index >= barcos_total) {
        return NULL;
    }
    return &barcos[index];
}

// Cantidad de slots usados 
int barcos_count()
{
    return barcos_total;
}
