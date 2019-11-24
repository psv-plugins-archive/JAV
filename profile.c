/*
自動オーディオボリューム - profile.c
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
#include "profile.h"
#include "log.h"

#define CONFIG_PATH "ur0:data/jav.cfg"

audio_profile_t file_profile;
audio_profile_t profile;

void reset_profile(void) {
	sceClibMemset(&file_profile, 0xFF, sizeof(file_profile));
	sceClibMemset(&profile, 0, sizeof(profile));
	profile.avls = get_avls();
	profile.muted = get_muted();
	int cur_vol = get_volume();
	profile.volumes[SPEAKER] = cur_vol;
	profile.volumes[HEADPHONE] = cur_vol;
	LOG("profile reset\n");
}

int load_profile(void) {
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0777);
	if (fd < 0) { goto fail; }

	int ret = sceIoRead(fd, &file_profile, sizeof(file_profile));
	sceIoClose(fd);
	if (ret != sizeof(file_profile)) { goto fail; }

	if (file_profile.avls != 0 && file_profile.avls != 1) { goto fail; }
	if (file_profile.muted != 0 && file_profile.muted != 1) { goto fail; }
	for (int i = 0; i < N_OUTPUTS; i++) {
		if (file_profile.volumes[i] < 0 || 30 < file_profile.volumes[i]) {
			goto fail;
		}
	}

	sceClibMemcpy(&profile, &file_profile, sizeof(profile));
	LOG("profile loaded\n");
	return 0;

fail:
	sceClibMemset(&file_profile, 0xFF, sizeof(file_profile));
	LOG("profile failed to load\n");
	return -1;
}

int write_profile(void) {
	if (sceClibMemcmp(&profile, &file_profile, sizeof(profile)) == 0) {
		return 0;
	}

	sceIoMkdir("ur0:data/", 0777);
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	if (fd < 0) { return -1; }

	int ret = sceIoWrite(fd, &profile, sizeof(profile));
	sceIoClose(fd);
	if (ret != sizeof(profile)) {
		LOG("profile failed to write\n");
		return -1;
	} else {
		sceClibMemcpy(&file_profile, &profile, sizeof(file_profile));
		LOG("profile written to file\n");
		return 0;
	}
}

void apply_profile(int output) {
	if (profile.avls) {
		for (int i = 0; i < N_OUTPUTS; i++) {
			int vol = profile.volumes[i];
			profile.volumes[i] = vol <= AVLS_MAX ? vol : AVLS_MAX;
		}
	}
	set_avls(profile.avls);
	set_volume(profile.volumes[output]);
	if (profile.muted) { mute_on(); }
	LOG("profile output %d applied\n", output);
	LOG("avls: %d muted: %d vol: %d\n", profile.avls, profile.muted, profile.volumes[output]);
}
