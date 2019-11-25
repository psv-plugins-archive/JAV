#ifndef CONFIG_H
#define CONFIG_H

#include "audio.h"

typedef struct {
	int avls;
	int muted;
	int volumes[N_OUTPUTS];
} jav_config_t;

extern jav_config_t config;

void reset_config(void);
int read_config(void);
int write_config(void);
void load_config(int output);

#endif
