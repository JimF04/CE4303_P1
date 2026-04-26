#include <stdio.h>
#include <string.h>
#include "barco.h"

static barco_t barcos[20];
static int barcos_total = 0;

// ========================
//   TAREA DE CADA BARCO
// ========================
static void barco_task(void *arg)
{
    barco_t *b = (barco_t *)arg;

    printf("Barco %d (%s) creado, esperando permiso...\n",
           b->id, b->tipo);

    while (1) {

        // Espera a que el scheduler lo despierte
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // → Aquí avanza 1 unidad
        b->posicion += b->velocidad;

        printf("Barco %d avanza a posición %d\n",
               b->id, b->posicion);

        // Luego se bloquea de nuevo hasta que el scheduler lo llame
    }
}

// ========================
//   CREACIÓN DE BARCOS
// ========================
void barcos_init(const config_t *cfg)
{
    int id = 0;

    // ------ BARCOS DE LA IZQUIERDA ------
    for (int i = 0; i < cfg->barcos.cantidad; i++) {
        barco_t *b = &barcos[id];
        b->id = id;
        strncpy(b->tipo, cfg->barcos.izquierda[i], 16);
        b->direccion = 0;
        b->posicion = 0;
        b->velocidad = cfg->barcos.velocidad_base;

        xTaskCreate(barco_task, "barco_L", 4096, b, 2, &b->handle);
        id++;
    }

    // ------ BARCOS DE LA DERECHA ------
    for (int i = 0; i < cfg->barcos.cantidad; i++) {
        barco_t *b = &barcos[id];
        b->id = id;
        strncpy(b->tipo, cfg->barcos.derecha[i], 16);
        b->direccion = 1;
        b->posicion = 0;
        b->velocidad = cfg->barcos.velocidad_base;

        xTaskCreate(barco_task, "barco_R", 4096, b, 2, &b->handle);
        id++;
    }

    barcos_total = id;

    printf("Se crearon %d barcos\n", barcos_total);
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