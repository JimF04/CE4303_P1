#include <stdio.h>
#include <string.h>
#include "barco.h"
#include <math.h>

#include <stdbool.h>


static barco_t barcos[8];
static int barcos_total = 0;
static int id_global = 0;

// ========================
//   TAREA DE CADA BARCO
// ========================
void barco_task(void *arg)
{
    barco_t *b = (barco_t *)arg;

    while (1) {
        // Espera turno del scheduler
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        b->state = RUNNING;

        printf("[RUNNING] %s ejecuta\n", b->nombre);

        //Simular uso de CPU (NO mover barco)
        vTaskDelay(pdMS_TO_TICKS(50));

        //Volver a READY (el canal maneja movimiento real)
        if (b->state != DONE) {
            b->state = READY;
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

    barco_t *b = &barcos[barcos_total];
    b->id = id_global;

    strncpy(b->tipo, tipo_b, sizeof(b->tipo));
    b->tipo[15] = '\0';

    b->direccion = direccion_b;
    b->pos_canal = -1;
    b->state = READY;

    asignar_velocidad(cfg, b);

    char dir = (direccion_b == 0) ? 'L' : 'R';
    snprintf(b->nombre, sizeof(b->nombre), "%s_%c_%d", tipo_b, dir, b->id);

    xTaskCreate(barco_task, b->nombre, 4096, b, 2, &b->handle);

    id_global++;
    barcos_total++;

    printf("Barco %d creado (%s)\n", b->id, b->tipo);

    return true;  // ✅ éxito
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
    if (index < 0 || index >= barcos_total) return;

    barco_t *b = &barcos[index];

    if (b->handle != NULL) {
        vTaskDelete(b->handle);
        b->handle = NULL;  // 🔥 CLAVE
    }

    b->state = DONE;
    b->pos_canal = -1;
    b->id = -1;

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
