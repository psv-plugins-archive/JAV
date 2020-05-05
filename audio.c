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

#include <psp2/avconfig.h>
#include <psp2/registrymgr.h>
#include "sce_avconfig.h"
#include "sce_bt.h"
#include "audio.h"
#include "util.h"
#include "log.h"

#define SOUND_REG "/CONFIG/SOUND"

static int get_connected_bt(int *mac0, int *mac1) {
	SceBtRegisteredInfo bt_info[N_DEVICE_BLUETOOTH];
	int ret = sceBtGetRegisteredInfo(0, 0, bt_info, N_DEVICE_BLUETOOTH);
	for (int i = 0; i < ret; i++) {
		SceBtRegisteredInfo *j = bt_info + i;
		int con_info = sceBtGetConnectingInfo(j->mac0, j->mac1);
		if (j->bt_profile & BT_PROFILE_AVRCP && con_info == BT_CON_CONNECTED_AUDIO_OUT) {
			*mac0 = j->mac0;
			*mac1 = j->mac1;
			return 0;
		}
	}
	return -1;
}

int get_device(int *mac0, int* mac1) {
	int r0, r1, r2, dev;
	*mac0 = *mac1 = 0;

	RLZ(sceAVConfigGetConnectedAudioDevice(&r0));
	if (r0 == (AVCONFIG_CONDEV_VITA_0 | AVCONFIG_CONDEV_VITA_8)) {
		dev = SPEAKER;
	} else if (r0 & AVCONFIG_CONDEV_BT_AUDIO_OUT) {
		dev = BLUETOOTH;
	} else if (r0 & AVCONFIG_CONDEV_AUDIO_OUT) {
		dev = HEADPHONE;
	} else {
		return -1;
	}

	RLZ(sceAVConfigGetVolCtrlEnable(&r0, &r1, &r2));
	if ((dev == SPEAKER || dev == HEADPHONE) && r0 == AVCONFIG_VOLCTRL_ONBOARD) {
		return dev;
	} else if (dev == BLUETOOTH
			&& r0 == AVCONFIG_VOLCTRL_BLUETOOTH
			&& get_connected_bt(mac0, mac1) == 0) {
		return dev;
	} else {
		return -1;
	}
}

int get_ob_volume(void) {
	int vol;
	RLZ(sceAVConfigGetSystemVol(&vol));
	return vol;
}

int set_ob_volume(int vol) {
	// sceAVConfigSetSystemVol can return 0 but volume is still not set
	int current = get_ob_volume();
	RLZ(current);
	if (current != vol) {
		RLZ(sceAVConfigSetSystemVol(vol));
		RNE(get_ob_volume(), vol);
		RLZ(sceAVConfigWriteRegSystemVol(vol));
	}
	return 0;
}

int get_muted(void) {
	int r1, muted, r3;
	RLZ(sceAVConfigGetVolCtrlEnable(&r1, &muted, &r3));
	return muted;
}

int mute_on(void) {
	// sceAVConfigMuteOn can return 0 but still not muted
	int current = get_muted();
	RLZ(current);
	if (current != 1) {
		RLZ(sceAVConfigMuteOn());
		RNE(get_muted(), 1);
	}
	return 0;
}

int get_avls(void) {
	int r1, r2, avls;
	RLZ(sceAVConfigGetVolCtrlEnable(&r1, &r2, &avls));
	return avls;
}

int set_avls(int v) {
	int current = get_avls();
	RLZ(current);
	if (current != v) {
		RLZ(sceAVConfigChangeReg(AVCONFIG_REG_AVLS, v));
		RNE(get_avls(), v);
		RLZ(sceRegMgrSetKeyInt(SOUND_REG, "avls", v));
	}
	return 0;
}

int disable_avls_timer(void) {
	int ret;
	RLZ(sceAVConfigChangeReg(AVCONFIG_REG_AVLS_TIMER, 0));
	RLZ(sceRegMgrGetKeyInt(SOUND_REG, "avls_timer", &ret));
	if (ret > 0) { RLZ(sceRegMgrSetKeyInt(SOUND_REG, "avls_timer", 0)); }
	return 0;
}

int get_speaker_mute(void) {
	int ret;
	RLZ(sceRegMgrGetKeyInt(SOUND_REG, "speaker_mute", &ret));
	return ret;
}

int set_speaker_mute(int v) {
	int current = get_speaker_mute();
	RLZ(current);
	if (current != v) {
		RLZ(sceAVConfigChangeReg(AVCONFIG_REG_SPEAKER_MUTE, v));
		RNE(get_speaker_mute(), v);
		RLZ(sceRegMgrSetKeyInt(SOUND_REG, "speaker_mute", v));
	}
	return 0;
}

int get_bt_volume(int mac0, int mac1) {
	int vol;
	RLZ(vol = sceBtAvrcpReadVolume(mac0, mac1));
	return BT2OB(vol);
}

int set_bt_volume(int mac0, int mac1, int vol) {
	int current = get_bt_volume(mac0, mac1);
	RLZ(current);
	if (current != vol) {
		vol = OB2BT(vol);
		RLZ(sceBtAvrcpSendVolume(mac0, mac1, vol));
		RNE(sceBtAvrcpReadVolume(mac0, mac1), vol);
	}
	return 0;
}
