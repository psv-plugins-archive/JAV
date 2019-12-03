/*
自動オーディオボリューム - config.c
Copyright (C) 2019 浅倉麗子

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

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include "config.h"
#include "log.h"

#define CONFIG_PATH "ur0:data/jav.cfg"

static jav_config_t file_config;
jav_config_t config;

void reset_config(void) {
	sceClibMemset(&file_config, 0xFF, sizeof(file_config));
	sceClibMemset(&config, 0, sizeof(config));
	config.avls = get_avls();
	config.muted = get_muted();
	for (int i = 0; i < N_OUTPUTS_ONBOARD; i++) {
		config.ob_volume[i] = 0;
	}
	LOG("config reset\n");
}

int read_config(void) {
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0777);
	if (fd < 0) { goto fail; }

	int ret = sceIoRead(fd, &file_config, sizeof(file_config));
	sceIoClose(fd);
	if (ret != sizeof(file_config)) { goto fail; }

	if (file_config.avls != 0 && file_config.avls != 1) { goto fail; }
	if (file_config.muted != 0 && file_config.muted != 1) { goto fail; }
	for (int i = 0; i < N_OUTPUTS_ONBOARD; i++) {
		if (file_config.ob_volume[i] < 0 || 30 < file_config.ob_volume[i]) {
			goto fail;
		}
	}

	sceClibMemcpy(&config, &file_config, sizeof(config));
	LOG("config loaded\n");
	return 0;

fail:
	sceClibMemset(&file_config, 0xFF, sizeof(file_config));
	LOG("config failed to load\n");
	return -1;
}

int write_config(void) {
	if (sceClibMemcmp(&config, &file_config, sizeof(config)) == 0) {
		return 0;
	}

	sceIoMkdir("ur0:data/", 0777);
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	if (fd < 0) { return -1; }

	int ret = sceIoWrite(fd, &config, sizeof(config));
	sceIoClose(fd);
	if (ret != sizeof(config)) {
		LOG("config failed to write\n");
		return -1;
	} else {
		sceClibMemcpy(&file_config, &config, sizeof(file_config));
		LOG("config written to file\n");
		return 0;
	}
}

void load_config(int output) {
	if (config.avls) {
		for (int i = 0; i < N_OUTPUTS_ONBOARD; i++) {
			int vol = config.ob_volume[i];
			config.ob_volume[i] = vol <= AVLS_MAX ? vol : AVLS_MAX;
		}
	}
	set_avls(config.avls);
	set_ob_volume(config.ob_volume[output]);
	if (config.muted) { mute_on(); }
	LOG("config output %d applied\n", output);
	LOG("avls: %d muted: %d vol: %d\n", config.avls, config.muted, config.ob_volume[output]);
}
