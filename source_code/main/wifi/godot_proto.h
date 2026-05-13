#ifndef MAIN_WIFI_GODOT_PROTO_H_
#define MAIN_WIFI_GODOT_PROTO_H_

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
    // Aumentamos un poco el buffer por seguridad
    char *json_buf = malloc(2560); 
    if (!json_buf) return;

    // Obtenemos la cola actual para extraer el buque activo y la lista
    barco_t *cola[10];
    int n = sched->get_queue(c->direccion_actual, cola, 10);
    
    // El "buque_act" es el primero en la cola (índice 0)
    int proximo_id = (n > 0 && cola[0] != NULL) ? cola[0]->id : -1;

    int pos = 0;
    pos += sprintf(json_buf + pos, "{");
    
    // 1. Info General
    pos += sprintf(json_buf + pos, "\"dir\":%d,\"buque_act\":%d,", 
                   c->direccion_actual, 
                   proximo_id);

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

    // 3. Lista ordenada
    pos += sprintf(json_buf + pos, "\"ordenado\":[");
    for (int i = 0; i < n; i++) {
        pos += sprintf(json_buf + pos, "%d%s", cola[i]->id, (i < n - 1) ? "," : "");
    }
    pos += sprintf(json_buf + pos, "]}");

    // Enviar por WebSocket
    send_to_all_ws(json_buf);

    free(json_buf);
}

#endif /* MAIN_WIFI_GODOT_PROTO_H_ */
