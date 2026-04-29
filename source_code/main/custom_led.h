#ifndef MAIN_CUSTOM_LED_H_
#define MAIN_CUSTOM_LED_H_

#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "canal/canal.h"
#include "barcos/barco.h"
#include "scheduler/scheduler.h"

#define UART_LED_PORT   UART_NUM_0
#define LED_TOTAL       30

#define LED_IZQ_START    0
#define LED_IZQ_END      3
#define LED_CANAL_START  5
#define LED_CANAL_END   24
#define LED_DER_START   26
#define LED_DER_END     29
#define LED_CANAL_COUNT (LED_CANAL_END - LED_CANAL_START + 1)  // 20 LEDs

// ── UART init ─────────────────────────────────────────────
void uart_init_led(void)
{
    uart_config_t cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE
    };
    uart_driver_install(UART_LED_PORT, 2048, 0, 0, NULL, 0);
    uart_param_config(UART_LED_PORT, &cfg);
    uart_set_pin(UART_LED_PORT, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// ── Comandos básicos (uso opcional) ──────────────────────
void send_led(int pos, int r, int g, int b)
{
    char buf[32];
    int  len = snprintf(buf, sizeof(buf), "LED %d %d %d %d\n", pos, r, g, b);
    uart_write_bytes(UART_LED_PORT, buf, len);
}

void send_all(int r, int g, int b)
{
    char buf[24];
    int  len = snprintf(buf, sizeof(buf), "ALL %d %d %d\n", r, g, b);
    uart_write_bytes(UART_LED_PORT, buf, len);
}

void clear_all(void)
{
    uart_write_bytes(UART_LED_PORT, "CLR\n", 4);
}

// ── Color según tipo ──────────────────────────────────────
static void color_por_tipo(const char *tipo, int *r, int *g, int *b)
{
    if      (strcmp(tipo, "NOR") == 0) { *r = 0;   *g = 0;   *b = 255; }
    else if (strcmp(tipo, "PES") == 0) { *r = 0;   *g = 255; *b = 0;   }
    else if (strcmp(tipo, "PAT") == 0) { *r = 255; *g = 0;   *b = 0;   }
    else                               { *r = 255; *g = 255; *b = 255; }
}

// ── Mapeo pos_canal → LED ─────────────────────────────────
static int canal_pos_to_led(int pos_canal, int largo)
{
    if (largo <= 1) return LED_CANAL_START;

    int led = LED_CANAL_START +
              (pos_canal * (LED_CANAL_COUNT - 1)) / (largo - 1);

    if (led < LED_CANAL_START) led = LED_CANAL_START;
    if (led > LED_CANAL_END)   led = LED_CANAL_END;

    return led;
}

// ── Render completo via FRAME ─────────────────────────────
// Agregar parámetro sched
void led_render_canal(const canal_t *c, const scheduler_t *sched)
{
    int R[LED_TOTAL] = {0};
    int G[LED_TOTAL] = {0};
    int B[LED_TOTAL] = {0};

    // ── Barcos en el canal ────────────────────────────────
    for (int i = 0; i < c->largo; i++) {
        barco_t *b = c->slots[i];
        if (!b) continue;
        int led = canal_pos_to_led(i, c->largo);
        color_por_tipo(b->tipo, &R[led], &G[led], &B[led]);
    }

    // ── Cola izquierda: primero = más cercano al canal (LED 3) ──
    barco_t *cola_izq[BARCOS_MAX];
    int n_izq = sched->get_queue(0, cola_izq, BARCOS_MAX);

    for (int i = 0; i < n_izq && (LED_IZQ_END - i) >= LED_IZQ_START; i++) {
        barco_t *b = cola_izq[i];
        if (!b) continue;
        int led = LED_IZQ_END - i;   // primero → LED 3, segundo → LED 2 ...
        int r, g, bl;
        color_por_tipo(b->tipo, &r, &g, &bl);
        R[led] = r  / 3;
        G[led] = g  / 3;
        B[led] = bl / 3;
    }

    // ── Cola derecha: primero = más cercano al canal (LED 25) ──
    barco_t *cola_der[BARCOS_MAX];
    int n_der = sched->get_queue(1, cola_der, BARCOS_MAX);

    for (int i = 0; i < n_der && (LED_DER_START + i) <= LED_DER_END; i++) {
        barco_t *b = cola_der[i];
        if (!b) continue;
        int led = LED_DER_START + i;  // primero → LED 25, segundo → LED 26 ...
        int r, g, bl;
        color_por_tipo(b->tipo, &r, &g, &bl);
        R[led] = r  / 3;
        G[led] = g  / 3;
        B[led] = bl / 3;
    }

    // ── Construir y enviar FRAME ──────────────────────────
    char frame[512];
    int  pos = snprintf(frame, sizeof(frame), "FRAME ");

    for (int i = 0; i < LED_TOTAL; i++) {
        pos += snprintf(frame + pos, sizeof(frame) - pos,
                        "%d,%d,%d%s",
                        R[i], G[i], B[i],
                        i < LED_TOTAL - 1 ? ";" : "\n");
    }

    uart_write_bytes(UART_LED_PORT, frame, pos);
}

#endif /* MAIN_CUSTOM_LED_H_ */