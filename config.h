#ifndef CONFIG_H
#define CONFIG_H

#include "audio.h"

typedef struct {
	int avls;
	int muted;
	int ob_volume[N_DEVICE_ONBOARD];
} jav_config_t;

extern jav_config_t config;

void reset_config(void);

int read_config(void);
int write_config(void);

void load_config(int device);

#endif
