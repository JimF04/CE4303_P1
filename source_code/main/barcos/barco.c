#include <stdio.h>
#include <string.h>
#include "barco.h"
#include <math.h>

#define BARCOS_MAX 8

static barco_t barcos[8];
static int barcos_total = 0;
static int id_global = 0;

// ========================
//   TAREA DE CADA BARCO
// ========================
static void barco_task(void *arg)
{
    barco_t *b = (barco_t *)arg;

    // BLOQUEO INICIAL
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (1) {

        // El barco NO se mueve solo
        printf("%s ejecuta (vel=%d)\n",
               b->nombre, b->velocidad);

        // Espera siguiente quantum
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

// ========================
//   CREACIÓN DE BARCOS
// ========================

// Metodo para crear un barco (proceso)
void crear_barco(const config_t *cfg, const char tipo_b[16], int direccion_b)
{
	int capacidad_logica = cfg->barcos.cantidad * 2;

	if (capacidad_logica > BARCOS_MAX) {
	    printf("ERROR: config.ini solicita %d barcos (máximo permitido = %d)\n",
	           capacidad_logica, BARCOS_MAX);
	    return;
	}

	if (barcos_total >= capacidad_logica) {
	    printf("Límite del config alcanzado (%d barcos)\n", capacidad_logica);
	    return;
	}

	if (barcos_total >= BARCOS_MAX) {
	    printf("Límite físico alcanzado (%d barcos)\n", BARCOS_MAX);
	    return;
	}
	

	barco_t *b = &barcos[barcos_total];
	b->id = id_global;

    strncpy(b->tipo, tipo_b, sizeof(b->tipo));
    b->tipo[15] = '\0';

    b->direccion = direccion_b;
    b->pos_canal = -1;

    // Asignar velocidad correctamente
    asignar_velocidad(cfg, b);

	// Crear el nombre del barco (tipo + dirección + id)
	char dir = (direccion_b == 0) ? 'L' : 'R';
	snprintf(b->nombre, sizeof(b->nombre), "%s_%c_%d", tipo_b, dir, b->id);

	// Crear el task con nombre 
	xTaskCreate(barco_task, b->nombre, 4096, b, 2, &b->handle);

    id_global++;
    barcos_total++;

    printf("Barco %d creado (%s)\n", b->id, b->tipo);

 
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
//   ELIMINACION DE BARCOS
// ========================

void eliminar_barco(int index)
{
    if (index < 0 || index >= barcos_total) return;

    barco_t *b = &barcos[index];

    if (b->handle != NULL) {
        vTaskDelete(b->handle);
    }

    // Compactar array
    for (int i = index; i < barcos_total - 1; i++) {
        barcos[i] = barcos[i + 1];
    }

    barcos_total--;

    printf("Barco eliminado\n");
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