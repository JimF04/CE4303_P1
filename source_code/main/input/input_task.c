#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#include "barcos/barco.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"
#include "input/input_task.h"

extern config_t config;
extern scheduler_t sched;
extern canal_t *canal_global;

// Pines
#define PIN_INTE GPIO_NUM_0
#define PIN_BAR  GPIO_NUM_10
#define PIN_BAR2 GPIO_NUM_8

#define LDR_CHANNEL ADC_CHANNEL_0
#define LDR_UMBRAL 300

#define MULTI_PRESS_WINDOW_MS 800

// Notificaciones
#define EVT_BAR   (1 << 0)
#define EVT_BAR2  (1 << 1)

// Handle del task
static TaskHandle_t input_task_handle = NULL;

//-----------------------------------------
// ISR
//-----------------------------------------
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (gpio_num == PIN_BAR) {
        xTaskNotifyFromISR(input_task_handle, EVT_BAR,
                           eSetBits, &xHigherPriorityTaskWoken);
    }
    else if (gpio_num == PIN_BAR2) {
        xTaskNotifyFromISR(input_task_handle, EVT_BAR2,
                           eSetBits, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

//-----------------------------------------
void crear_barco_por_pulsos(int pulsos, int direccion)
{
    char tipo[16];

    if (pulsos >= 3) strcpy(tipo, "PAT");
    else if (pulsos == 2) strcpy(tipo, "PES");
    else strcpy(tipo, "NOR");

    printf("Creando barco tipo: %s, direccion: %d\n",
           tipo, direccion);

    if (crear_barco(&config, tipo, direccion)) {
        int count = barcos_count();

        if (count > 0) {
            barco_t *b = barcos_get(count - 1);
            if (b != NULL) sched.enqueue(b);
        }
    }
}

//-----------------------------------------
void input_task(void *arg)
{
    input_task_handle = xTaskGetCurrentTaskHandle();

    //-----------------------------------------
    // GPIO con interrupciones
    //-----------------------------------------
    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << PIN_BAR) |
            (1ULL << PIN_BAR2),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(PIN_BAR, gpio_isr_handler, (void*) PIN_BAR);
    gpio_isr_handler_add(PIN_BAR2, gpio_isr_handler, (void*) PIN_BAR2);

    //-----------------------------------------
    // ADC
    //-----------------------------------------
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    adc_oneshot_config_channel(adc_handle, LDR_CHANNEL, &adc_config);

    //-----------------------------------------
    // Estados
    //-----------------------------------------
    int last_level_int = 0;

    int bar_press_count = 0;
    TickType_t first_press_time = 0;

    int bar2_press_count = 0;
    TickType_t first_press_time2 = 0;

    while (1) {

        //-----------------------------------------
        // Esperar eventos (timeout para LDR)
        //-----------------------------------------
        uint32_t events = 0;

        xTaskNotifyWait(
            0,          // no limpiar al entrar
            0xFFFFFFFF, // limpiar al salir
            &events,
            pdMS_TO_TICKS(50)
        );

        TickType_t now = xTaskGetTickCount();

        //-----------------------------------------
        // EVENTOS BOTONES
        //-----------------------------------------
        if (events & EVT_BAR) {

            if (bar_press_count == 0) {
                first_press_time = now;
            }

            bar_press_count++;
            printf("Pulso BAR (%d)\n", bar_press_count);
        }

        if (events & EVT_BAR2) {

            if (bar2_press_count == 0) {
                first_press_time2 = now;
            }

            bar2_press_count++;
            printf("Pulso BAR2 (%d)\n", bar2_press_count);
        }

        //-----------------------------------------
        // LDR (se mantiene polling ligero)
        //-----------------------------------------
        int ldr_value = 0;

        adc_oneshot_read(adc_handle, LDR_CHANNEL, &ldr_value);

        if (ldr_value < LDR_UMBRAL && last_level_int == 0) {
            canal_viene_buque(canal_global);
            last_level_int = 1;
        }
        else if (ldr_value >= LDR_UMBRAL) {
            last_level_int = 0;
        }

        //-----------------------------------------
        // Resolver BAR
        //-----------------------------------------
        if (bar_press_count > 0 &&
            (now - first_press_time) >=
            pdMS_TO_TICKS(MULTI_PRESS_WINDOW_MS))
        {
            crear_barco_por_pulsos(bar_press_count, 0);
            bar_press_count = 0;
        }

        //-----------------------------------------
        // Resolver BAR2
        //-----------------------------------------
        if (bar2_press_count > 0 &&
            (now - first_press_time2) >=
            pdMS_TO_TICKS(MULTI_PRESS_WINDOW_MS))
        {
            crear_barco_por_pulsos(bar2_press_count, 1);
            bar2_press_count = 0;
        }
    }
}