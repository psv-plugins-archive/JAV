/*
This file is part of JAV 自動オーディオボリューム
Copyright © 2019, 2020 浅倉麗子

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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

void reset_config(jav_config_t *config);

int read_config(jav_config_t *config);
int write_config(jav_config_t *config);

int load_config(jav_config_t *config, int device, int mac0, int mac1);
int save_config(jav_config_t *config, int device, int mac0, int mac1);

#endif
