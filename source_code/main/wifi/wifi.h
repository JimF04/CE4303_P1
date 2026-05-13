#ifndef MAIN_WIFI_WIFI_H_
#define MAIN_WIFI_WIFI_H_


#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "mdns.h"

static const char *W_TAG = "WIFI_WS";
static httpd_handle_t server = NULL;

// 1. Nueva función para configurar el nombre de red
static void start_mdns_service(void) {
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(W_TAG, "mDNS Init falló: %d", err);
        return;
    }

    // El nombre que usarás en Godot será: canal.local
    mdns_hostname_set("canal"); 
    mdns_instance_name_set("ESP32 Canal Sched");

    // Agregamos el servicio para que sea "descubrible"
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    ESP_LOGI(W_TAG, "mDNS configurado como: canal.local");
}

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) return ESP_OK;

    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) return ret;

    if (frame.len > 0) {
        uint8_t *buf = malloc(frame.len + 1);
        frame.payload = buf;
        httpd_ws_recv_frame(req, &frame, frame.len);
        buf[frame.len] = 0;
        ESP_LOGI(W_TAG, "Godot dice: %s", (char*)buf);
        free(buf);
    }
    return ESP_OK;
}

static void wifi_init(const char* ssid, const char* password) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.rssi = -127,
        },
    };
    
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    
    ESP_LOGI(W_TAG, "Conectando a SSID: %s", ssid);

    // 2. Iniciamos mDNS justo después de conectar
    start_mdns_service();
}

static void start_ws_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = ws_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);
        ESP_LOGI(W_TAG, "Servidor WS en: ws://canal.local/ws");
    }
}

#endif /* MAIN_WIFI_WIFI_H_ */
