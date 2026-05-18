#ifndef MAIN_CUSTOM_LED_H_
#define MAIN_CUSTOM_LED_H_

#include <string.h>
#include "led_strip.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "canal/canal.h"
#include "barcos/barco.h"
#include "scheduler/scheduler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ── Configuración física ──────────────────────────────────────────────────────
#define LED_GPIO         22
#define LED_TOTAL        29
#define LED_BRIGHTNESS   25    

#define LED_IZQ_START    0
#define LED_IZQ_END      3
#define LED_CANAL_START  4
#define LED_CANAL_END    23
#define LED_DER_START    24
#define LED_DER_END      27
#define LED_CANAL_COUNT (LED_CANAL_END - LED_CANAL_START + 1)   // 20 LEDs

// ── Handle global del strip ───────────────────────────────────────────────────
static led_strip_handle_t s_strip = NULL;

// ── Init ──────────────────────────────────────────────────────────────────────
void led_strip_init_custom(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num         = LED_GPIO,
        .max_leds               = LED_TOTAL,
        .led_model              = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,  
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_cfg = {
        .clk_src         = RMT_CLK_SRC_DEFAULT,
        .resolution_hz   = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip));
    led_strip_clear(s_strip);
}

// ── Helpers internos ──────────────────────────────────────────────────────────

// Aplica el factor de brillo global sobre un canal
static inline uint8_t _bright(int val)
{
    return (uint8_t)((val * LED_BRIGHTNESS) / 255);
}

static void _set_pixel(int pos, int r, int g, int b)
{
    if (pos < 0 || pos >= LED_TOTAL) return;
    led_strip_set_pixel(s_strip, pos, _bright(r), _bright(g), _bright(b));
}

static void _clear_all(void)
{
    led_strip_clear(s_strip);
}

// Color según tipo de barco 
static void color_por_tipo(const char *tipo, int *r, int *g, int *b)
{
    if      (strcmp(tipo, "NOR") == 0) { *r = 0;   *g = 0;   *b = 255; } // Azul
    else if (strcmp(tipo, "PES") == 0) { *r = 0;   *g = 255; *b = 0;   } // Verde
    else if (strcmp(tipo, "PAT") == 0) { *r = 255; *g = 0;   *b = 0;   } // Rojo
    else                               { *r = 255; *g = 255; *b = 255; } // Blanco
}

// Convierte una posición lógica dentro del canal (0..largo-1)
// a un índice físico de LED (LED_CANAL_START..LED_CANAL_END)
// usando interpolación lineal para distribuir uniformemente
static int canal_pos_to_led(int pos_canal, int largo)
{
    if (largo <= 1) return LED_CANAL_START;

    int led = LED_CANAL_START +
              (pos_canal * (LED_CANAL_COUNT - 1)) / (largo - 1);

    if (led < LED_CANAL_START) led = LED_CANAL_START;
    if (led > LED_CANAL_END)   led = LED_CANAL_END;
    return led;
}

// ── Render principal ──────────────────────────────────────────────────────────
void led_render_canal(const canal_t *c, const scheduler_t *sched)
{
    static int64_t last_buque_us = 0;

	// Obtener snapshot atómico del estado del canal
    canal_snapshot_t snap;
    canal_snapshot(c, &snap);

    int R[LED_TOTAL] = {0}, G[LED_TOTAL] = {0}, B[LED_TOTAL] = {0};

    // ── Colas siempre, buque o no ────────────────────────────────────────────
    barco_t *cola_izq[BARCOS_MAX];
    int n_izq = sched->get_queue(0, cola_izq, BARCOS_MAX);
    for (int i = 0; i < n_izq && (LED_IZQ_END - i) >= LED_IZQ_START; i++) {
        if (!cola_izq[i]) continue;
        int led = LED_IZQ_END - i;
        int r, g, bl;
        color_por_tipo(cola_izq[i]->tipo, &r, &g, &bl);
        R[led] = r/3; G[led] = g/3; B[led] = bl/3;
    }

    barco_t *cola_der[BARCOS_MAX];
    int n_der = sched->get_queue(1, cola_der, BARCOS_MAX);
    for (int i = 0; i < n_der && (LED_DER_START + i) <= LED_DER_END; i++) {
        if (!cola_der[i]) continue;
        int led = LED_DER_START + i;
        int r, g, bl;
        color_por_tipo(cola_der[i]->tipo, &r, &g, &bl);
        R[led] = r/3; G[led] = g/3; B[led] = bl/3;
    }

    // ── LED 28: dirección siempre ─────────────────────────────────────────────
    if (snap.direccion_actual == 0)      { R[28]=255; G[28]=165; B[28]=0;   }
    else if (snap.direccion_actual == 1) { R[28]=0;   G[28]=255; B[28]=255; }
    // si es -1 (sin dirección), queda apagado

    if (snap.pasa_buque) {
        // Animar canal con colores random, respetar colas ya calculadas
        int64_t now = esp_timer_get_time();
        if (now - last_buque_us >= 100 * 1000) {
            last_buque_us = now;
            for (int i = LED_CANAL_START; i <= LED_CANAL_END; i++) {
                R[i] = esp_random() % 256;
                G[i] = esp_random() % 256;
                B[i] = esp_random() % 256;
            }
            for (int i = 0; i < LED_TOTAL; i++) _set_pixel(i, R[i], G[i], B[i]);
            led_strip_refresh(s_strip);
        }
        return;
    }

    // ── Canal normal ──────────────────────────────────────────────────────────
    for (int i = 0; i < snap.largo; i++) {
        if (!snap.slots[i]) continue;
        int led = canal_pos_to_led(i, snap.largo);
        color_por_tipo(snap.slots[i]->tipo, &R[led], &G[led], &B[led]);
    }

    for (int i = 0; i < LED_TOTAL; i++) _set_pixel(i, R[i], G[i], B[i]);
    led_strip_refresh(s_strip);
}

#endif /* MAIN_CUSTOM_LED_H_ */