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

// Configuración del AP
#define EXAMPLE_ESP_WIFI_SSID      "ESP32_C6_AP"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_MAX_STA_CONN       4

static void start_mdns_service(void) {
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(W_TAG, "mDNS Init falló: %d", err);
        return;
    }
    // En modo AP, el ESP suele ser 192.168.4.1. 
    // mDNS permitirá conectar a: ws://canal.local/ws
    mdns_hostname_set("canal"); 
    mdns_instance_name_set("ESP32 Canal Sched");
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
        if (buf == NULL) return ESP_ERR_NO_MEM;
        frame.payload = buf;
        httpd_ws_recv_frame(req, &frame, frame.len);
        buf[frame.len] = 0;
        ESP_LOGI(W_TAG, "Mensaje recibido: %s", (char*)buf);
        free(buf);
    }
    return ESP_OK;
}

static void wifi_init_softap(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 1. Crear la interfaz AP en lugar de STA
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 2. Configurar los parámetros del Access Point
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 1, // Canal del WiFi
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(W_TAG, "SoftAP iniciado. SSID: %s Password: %s", 
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

    start_mdns_service();
}

static void start_ws_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // Importante: En modo AP, mDNS a veces tarda, 
    // puedes usar también la IP por defecto 192.168.4.1
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = ws_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);
        ESP_LOGI(W_TAG, "Servidor WS iniciado.");
    }
}

#endif /* MAIN_WIFI_WIFI_H_ */