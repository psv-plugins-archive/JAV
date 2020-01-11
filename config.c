/*
自動オーディオボリューム
Copyright (C) 2019-2020 浅倉麗子

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
#include "sce_bt.h"
#include "config.h"
#include "util.h"
#include "log.h"

static jav_config_t file_config;

static bt_volume_t *find_bt_volume(jav_config_t *config, int mac0, int mac1) {
	bt_volume_t *bt_vol = config->bt_volume;
	for (int i = 0; i < N_DEVICE_BLUETOOTH; i++) {
		if (bt_vol[i].mac0 == mac0 && bt_vol[i].mac1 == mac1) {
			return bt_vol + i;
		}
	}
	return NULL;
}

static int prune_unpaired_bt(jav_config_t *config) {
	int ret;
	SceBtRegisteredInfo bt_info[N_DEVICE_BLUETOOTH];
	RLZ(ret = sceBtGetRegisteredInfo(0, 0, bt_info, N_DEVICE_BLUETOOTH));

	bt_volume_t new_bt_vol[N_DEVICE_BLUETOOTH];
	sceClibMemset(new_bt_vol, 0, sizeof(new_bt_vol));
	bt_volume_t *i = new_bt_vol;
	for (int j = 0; j < ret; j++) {
		bt_volume_t *ret = find_bt_volume(config, bt_info[j].mac0, bt_info[j].mac1);
		if (ret) { *(i++) = *ret; }
	}

	sceClibMemcpy(config->bt_volume, new_bt_vol, sizeof(config->bt_volume));
	return 0;
}

void reset_config(jav_config_t *config) {
	sceClibMemset(&file_config, 0xFF, sizeof(file_config));
	sceClibMemset(config, 0, sizeof(*config));
	LOG("config reset\n");
}

int read_config(jav_config_t *config) {
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
	if (fd < 0) { goto fail; }

	int ret = sceIoRead(fd, &file_config, sizeof(file_config));
	sceIoClose(fd);
	if (ret != sizeof(file_config)) { goto fail; }

	if (file_config.avls != 0 && file_config.avls != 1) { goto fail; }
	if (file_config.muted != 0 && file_config.muted != 1) { goto fail; }
	for (int i = 0; i < N_DEVICE_ONBOARD; i++) {
		if (file_config.ob_volume[i] < 0 || 30 < file_config.ob_volume[i]) {
			goto fail;
		}
	}
	for (int i = 0; i < N_DEVICE_BLUETOOTH; i++) {
		if (file_config.bt_volume[i].volume < 0 || 30 < file_config.bt_volume[i].volume) {
			goto fail;
		}
	}

	sceClibMemcpy(config, &file_config, sizeof(*config));
	LOG("config loaded\n");
	return 0;

fail:
	sceClibMemset(&file_config, 0xFF, sizeof(file_config));
	LOG("config failed to load\n");
	return -1;
}

int write_config(jav_config_t *config) {
	if (sceClibMemcmp(config, &file_config, sizeof(*config)) == 0) {
		return 0;
	}

	sceIoMkdir(CONFIG_BASE_DIR, 0777);
	sceIoMkdir(CONFIG_JAV_DIR, 0777);
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	if (fd < 0) { return -1; }

	int ret = sceIoWrite(fd, config, sizeof(*config));
	sceIoClose(fd);
	if (ret != sizeof(*config)) {
		LOG("config failed to write\n");
		return -1;
	} else {
		sceClibMemcpy(&file_config, config, sizeof(file_config));
		LOG("config written to file\n");
		return 0;
	}
}

int load_config(jav_config_t *config, int device, int mac0, int mac1) {
	int vol;
	if (device == SPEAKER || device == HEADPHONE) {
		if (config->avls) {
			for (int i = 0; i < N_DEVICE_ONBOARD; i++) {
				int vol = config->ob_volume[i];
				config->ob_volume[i] = vol <= AVLS_MAX ? vol : AVLS_MAX;
			}
		}
		vol = config->ob_volume[device];
		LOG("load %d vol %d muted %d avls %d\n", device, vol, config->muted, config->avls);
		RLZ(set_avls(config->avls));
		RLZ(set_ob_volume(vol));
		if (config->muted) { RLZ(mute_on()); }
	} else { // device == BLUETOOTH
		bt_volume_t *ret = find_bt_volume(config, mac0, mac1);
		vol = ret ? ret->volume : 0;
		LOG("load %d %08X%04X vol %d\n", device, mac0, mac1, vol);
		RLZ(set_bt_volume(mac0, mac1, vol));
	}
	return vol;
}

int save_config(jav_config_t *config, int device, int mac0, int mac1) {
	int vol = -1;
	if (device == SPEAKER || device == HEADPHONE) {
		int avls, muted;
		RLZ(avls = get_avls());
		RLZ(muted = get_muted());
		RLZ(vol = get_ob_volume());
		RNE(get_device(&mac0, &mac1), device);
		config->avls = avls;
		config->muted = muted;
		config->ob_volume[device] = vol;
	} else { // device == BLUETOOTH
		RLZ(prune_unpaired_bt(config));
		for (int i = 0; i < N_DEVICE_BLUETOOTH; i++) {
			bt_volume_t *j = config->bt_volume + i;
			if ((j->mac0 == mac0 && j->mac1 == mac1)
					|| (j->mac0 == 0 && j->mac1 == 0)) {
				RLZ(vol = get_bt_volume(mac0, mac1));
				j->mac0 = mac0;
				j->mac1 = mac1;
				j->volume = vol;
				break;
			}
		}
	}
	return vol;
}
