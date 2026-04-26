#include "config.h"
#include "lib/ini.h"

// REFERENCE TO EMBEDDED FILE
extern const uint8_t config_ini_start[] asm("_binary_config_ini_start");
extern const uint8_t config_ini_end[]   asm("_binary_config_ini_end");

// =====================================================
// PARSER HANDLER
// =====================================================

static int handler(void* user, const char* section,
                   const char* name, const char* value)
{
    config_t* cfg = (config_t*)user;

    if (strcmp(section, "CANAL") == 0) {

        if      (strcmp(name, "largo") == 0)
            cfg->canal.largo = atoi(value);

        else if (strcmp(name, "metodo_flujo") == 0)
            strncpy(cfg->canal.metodo_flujo, value, sizeof(cfg->canal.metodo_flujo));

        else if (strcmp(name, "tiempo_letrero_ms") == 0)
            cfg->canal.tiempo_letrero_ms = atoi(value);

        else if (strcmp(name, "parametro_w") == 0)
            cfg->canal.parametro_w = atoi(value);
    }

    else if (strcmp(section, "SCHEDULER") == 0) {

        if      (strcmp(name, "algoritmo") == 0)
            strncpy(cfg->scheduler.algoritmo, value, sizeof(cfg->scheduler.algoritmo));

        else if (strcmp(name, "quantum_ms") == 0)
            cfg->scheduler.quantum_ms = atoi(value);

        else if (strcmp(name, "preemptivo") == 0)
            cfg->scheduler.preemptivo = atoi(value);
    }

    else if (strcmp(section, "BARCOS") == 0) {

        if      (strcmp(name, "cantidad") == 0)
            cfg->barcos.cantidad = atoi(value);

        else if (strcmp(name, "velocidad_base") == 0)
            cfg->barcos.velocidad_base = atoi(value);

        else if (strcmp(name, "cfg_default") == 0)
            cfg->barcos.cfg_default = atoi(value);

        else if (strcmp(name, "izquierda") == 0) {
            char tmp[128];
            strncpy(tmp, value, sizeof(tmp));
            char* token = strtok(tmp, ",");
            int i = 0;
            while (token && i < 10) {
                strncpy(cfg->barcos.izquierda[i], token, sizeof(cfg->barcos.izquierda[i]));
                token = strtok(NULL, ",");
                i++;
            }
        }

        else if (strcmp(name, "derecha") == 0) {
            char tmp[128];
            strncpy(tmp, value, sizeof(tmp));
            char* token = strtok(tmp, ",");
            int i = 0;
            while (token && i < 10) {
                strncpy(cfg->barcos.derecha[i], token, sizeof(cfg->barcos.derecha[i]));
                token = strtok(NULL, ",");
                i++;
            }
        }
    }

    return 1;
}

// =====================================================
// PUBLIC API: config_load
// =====================================================

int config_load(config_t* cfg)
{
    size_t file_size = config_ini_end - config_ini_start;

    char* ini_data = malloc(file_size + 1);
    memcpy(ini_data, config_ini_start, file_size);
    ini_data[file_size] = '\0';

    int result = ini_parse_string(ini_data, handler, cfg);
    free(ini_data);

    return result;
}