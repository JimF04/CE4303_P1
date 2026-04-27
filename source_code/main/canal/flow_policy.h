#ifndef MAIN_CANAL_FLOW_POLICY_H_
#define MAIN_CANAL_FLOW_POLICY_H_

// flow_policy.h

#include "canal.h"
#include "config.h"

typedef struct flow_policy flow_policy_t;

struct flow_policy {
    void (*init)(flow_policy_t *self, canal_t *c, const config_t *cfg);

	// Puede entrar un barco de ese lado?
    int  (*allow_left)(flow_policy_t *self, canal_t *c);
    int  (*allow_right)(flow_policy_t *self, canal_t *c);

	// Llamado al final de cada tick (después de mover barcos)
    void (*tick)(flow_policy_t *self, canal_t *c);
	
	// Estado interno de la política (cada impl. usa su propio struct)
    void *state;
};

flow_policy_t *flow_policy_create(const char *mode, const config_t *cfg);


#endif /* MAIN_CANAL_FLOW_POLICY_H_ */
