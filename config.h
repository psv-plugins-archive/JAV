#ifndef CONFIG_H
#define CONFIG_H

#include "audio.h"

typedef struct {
	int mac0;
	int mac1;
	int volume;
} bt_volume_t;

typedef struct {
	int avls;
	int muted;
	int ob_volume[N_DEVICE_ONBOARD];
	bt_volume_t bt_volume[N_DEVICE_BLUETOOTH];
} jav_config_t;

extern jav_config_t config;

void reset_config(void);

int read_config(void);
int write_config(void);

int load_config(int device, int mac0, int mac1);
int save_config(int device, int mac0, int mac1);

#endif
