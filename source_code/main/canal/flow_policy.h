#ifndef MAIN_CANAL_FLOW_POLICY_H_
#define MAIN_CANAL_FLOW_POLICY_H_

// flow_policy.h

#include "canal.h"
#include "config.h"

typedef struct flow_policy flow_policy_t;

struct flow_policy {
    void (*init)(flow_policy_t *self, canal_t *c, const config_t *cfg);

    int  (*allow_left)(flow_policy_t *self, canal_t *c);
    int  (*allow_right)(flow_policy_t *self, canal_t *c);

    void (*tick)(flow_policy_t *self, canal_t *c);
    void *state;
};

flow_policy_t *flow_policy_create(const char *mode, const config_t *cfg);


#endif /* MAIN_CANAL_FLOW_POLICY_H_ */
