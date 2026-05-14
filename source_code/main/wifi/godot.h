#ifndef MAIN_WIFI_GODOT_H_
#define MAIN_WIFI_GODOT_H_

#include "esp_http_server.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"
#include "wifi.h"

#include "esp_http_server.h"
#include "canal/canal.h"
#include "scheduler/scheduler.h"
#include "wifi.h"

// Función para enviar el JSON por WebSocket
void send_to_all_ws(const char *payload) {
    if (server == NULL) return;

    size_t clients = 10;
    int client_fds[10];
    
    if (httpd_get_client_list(server, &clients, client_fds) == ESP_OK) {
        for (size_t i = 0; i < clients; i++) {
            if (httpd_ws_get_fd_info(server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
                httpd_ws_frame_t packet = {
                    .payload = (uint8_t *)payload,
                    .len = strlen(payload),
                    .type = HTTPD_WS_TYPE_TEXT
                };
                // Usamos send_frame_async para no bloquear el loop principal
                httpd_ws_send_frame_async(server, client_fds[i], &packet);
            }
        }
    }
}

void godot_export_state(const canal_t *c, const scheduler_t *sched) {
    // Buffer grande para las dos colas + slots
    char *json_buf = malloc(3072); 
    if (!json_buf) return;

    // Buffers temporales para capturar las dos colas del scheduler
    barco_t *cola_izq[10];
    barco_t *cola_der[10];
    int n_izq = sched->get_queue(0, cola_izq, 10); // 0 = Izquierda
    int n_der = sched->get_queue(1, cola_der, 10); // 1 = Derecha
    

    int pos = 0;
    pos += sprintf(json_buf + pos, "{");
    
    // 1. Info General
    pos += sprintf(json_buf + pos, "\"dir\":%d,\"buque_act\":%d,", 
                   c->direccion_actual, 
                   c->pasa_buque);

    // 2. Slots del Canal
    pos += sprintf(json_buf + pos, "\"slots\":[");
    for (int i = 0; i < c->largo; i++) {
        barco_t *b = c->slots[i];
        if (b) {
            pos += sprintf(json_buf + pos, "{\"id\":%d,\"tipo\":\"%s\"}", b->id, b->tipo);
        } else {
            pos += sprintf(json_buf + pos, "null");
        }
        if (i < c->largo - 1) pos += sprintf(json_buf + pos, ",");
    }
    pos += sprintf(json_buf + pos, "],");

	// 3. Lista ordenada Izquierda
	pos += sprintf(json_buf + pos, "\"ordenado_izq\":[");
	for (int i = 0; i < n_izq; i++) {
	    pos += sprintf(json_buf + pos, "{\"id\":%d,\"tipo\":\"%s\"}%s",
	                   cola_izq[i]->id,
	                   cola_izq[i]->tipo,
	                   (i < n_izq - 1) ? "," : "");
	}
	pos += sprintf(json_buf + pos, "],");

	// 4. Lista ordenada Derecha
	pos += sprintf(json_buf + pos, "\"ordenado_der\":[");
	for (int i = 0; i < n_der; i++) {
	    pos += sprintf(json_buf + pos, "{\"id\":%d,\"tipo\":\"%s\"}%s",
	                   cola_der[i]->id,
	                   cola_der[i]->tipo,
	                   (i < n_der - 1) ? "," : "");
	}
	pos += sprintf(json_buf + pos, "]}");

    send_to_all_ws(json_buf);
    free(json_buf);
}

#endif /* MAIN_WIFI_GODOT_H_ */
