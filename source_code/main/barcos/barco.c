#include <stdio.h>
#include <string.h>
#include "barco.h"

static barco_t barcos[20];
static int barcos_total = 0;
static int id_global = 0;

// ========================
//   TAREA DE CADA BARCO
// ========================
static void barco_task(void *arg)
{
    barco_t *b = (barco_t *)arg;

    printf("Barco %d (%s) creado. Esperando permiso...\n", 
            b->id, b->tipo);

    // BLOQUEO INICIAL
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (1) {

        // Cuando scheduler lo despierta -> avanza 1 unidad
        b->posicion += b->velocidad;
        printf("Barco %d avanza a posicion %d\n", 
                b->id, b->posicion);

        // Se bloquea otra vez esperando nuevo quantum
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

// ========================
//   CREACIÓN DE BARCOS
// ========================

// Metodo para crear un barco (proceso)
void crear_barco(const config_t *cfg, const char tipo_b[16], int direccion_b)
{
    barco_t *b = &barcos[id_global];

    b->id = id_global;

    strncpy(b->tipo, tipo_b, sizeof(b->tipo));
    b->tipo[15] = '\0';

    b->direccion = direccion_b;
    b->posicion = 0;

    // Asignar velocidad correctamente
    asignar_velocidad(cfg, b);

    // Nombre dinámico del task
    char task_name[32];
    snprintf(task_name, sizeof(task_name), "barco_%d", b->id);

    xTaskCreate(barco_task, task_name, 4096, b, 2, &b->handle);

    id_global++;
    barcos_total++;

    printf("Barco %d creado (%s)\n", b->id, b->tipo);
}


// Metodo para asignar la velocidad a los barcos
void asignar_velocidad(const config_t *cfg, barco_t *b)
{
    int base = cfg->barcos.velocidad_base;

    if (strcmp(b->tipo, "NORMAL") == 0) {
        b->velocidad = base;
    } 
    else if (strcmp(b->tipo, "PESQUERA") == 0) {
        b->velocidad = base + 1;
    } 
    else if (strcmp(b->tipo, "PATRULLA") == 0) {
        b->velocidad = base + 2;
    } 
    else {
        b->velocidad = base; // fallback
    }
}

// Metodo para crear barcos por default (depende del config)
void barcos_init(const config_t *cfg)
{
    id_global = 0;
    barcos_total = 0;

    // IZQUIERDA
    for (int i = 0; i < cfg->barcos.cantidad; i++) {
        crear_barco(cfg, cfg->barcos.izquierda[i], 0);
    }

    // DERECHA
    for (int i = 0; i < cfg->barcos.cantidad; i++) {
        crear_barco(cfg, cfg->barcos.derecha[i], 1);
    }

    printf("Total barcos creados: %d\n", barcos_total);
}

// ========================
//   ACCESORES
// ========================

barco_t* barcos_get(int index)
{
    return &barcos[index];
}

int barcos_count()
{
    return barcos_total;
}