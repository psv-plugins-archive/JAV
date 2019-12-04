/*
自動オーディオボリューム - main.c
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

// ＲＩＫＫＡ　ＰＲＯＪＥＣＴ
// Plugin idea: nkekev
// Testing: dots-tb, nkekev, ATTLAS
// Marketing: dots-tb
// Product manager: dots-tb
// Funded by: CBPS

#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>
#include <taihen.h>
#include "sce_shell.h"
#include "audio.h"
#include "config.h"
#include "log.h"

extern int ScePafToplevel_004D98CC(char *r1);
extern int ScePafToplevel_1DF2C6FD(int r1, int r2);

static SceUID top_func_enter_mtx = -1;
static SceUID top_func_exit_mtx = -1;

#define N_HOOKS 5
static SceUID hook_id[N_HOOKS];
static tai_hook_ref_t hook_ref[N_HOOKS];

static audio_info_t *audio_info = NULL;
static SceUID thread_id = -1;
static int run_thread = 1;

static int top_func(audio_info_t *a, button_info_t *p) {
	if (!audio_info) { audio_info = a; }

	if (sceKernelTryLockMutex(top_func_enter_mtx, 1) == 0) {
		int ret = TAI_CONTINUE(int, hook_ref[0], a, p);
		sceKernelUnlockMutex(top_func_enter_mtx, 1);
		return ret;
	}

	sceKernelLockMutex(top_func_exit_mtx, 1, NULL);
	sceKernelUnlockMutex(top_func_exit_mtx, 1);
	return 0;
}

static int init_vol_bar(volume_bar_t *v, int mode) {
	return TAI_CONTINUE(int, hook_ref[1], v, mode);
}

static int set_vol_bar_lvl(volume_bar_t *v, int level, int flags) {
	return TAI_CONTINUE(int, hook_ref[2], v, level, flags);
}

static int set_vol_bar_muted(volume_bar_t *v, int avls) {
	return TAI_CONTINUE(int, hook_ref[3], v, avls);
}

static int free_vol_bar(volume_bar_t *v) {
	return TAI_CONTINUE(int, hook_ref[4], v);
}

static void progress_vol_bar(int start, int end, int flag) {
	if (start < end) {
		for (int i = start; i <= end; i++) {
			set_vol_bar_lvl(&audio_info->vol_bar, i, flag);
			sceKernelDelayThread(20 * 1000);
		}
	} else if (start > end) {
		for (int i = start; i >= end; i--) {
			set_vol_bar_lvl(&audio_info->vol_bar, i, flag);
			sceKernelDelayThread(20 * 1000);
		}
	} else {
		set_vol_bar_lvl(&audio_info->vol_bar, start, flag);
	}
}

typedef int (*set_mute_icon_ptr)(int);

static int set_mute_icon(int v) {
	if (sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_PAF) < 0) {
		return -1;
	}
	int a = ScePafToplevel_004D98CC("indicator_plugin");
	if (a != 0 && (a = ScePafToplevel_1DF2C6FD(a, 1)) != 0) {
		(*(set_mute_icon_ptr*)(a + 0x20))(v);
		return 0;
	} else {
		return -1;
	}
}

static void SceShellMain_hang_enter(void) {
	for (;;) {
		sceKernelLockMutex(top_func_exit_mtx, 1, NULL);
		SceKernelMutexInfo info;
		while ((info.size = sizeof(info), sceKernelGetMutexInfo(top_func_exit_mtx, &info)) < 0
				|| info.numWaitThreads == 0) {
			sceKernelDelayThread(10 * 1000);
		}
		if (set_mute_icon(config.muted) == 0) {
			break;
		} else {
			sceKernelUnlockMutex(top_func_exit_mtx, 1);
			sceKernelDelayThread(50 * 1000);
		}
	}
	// set this field to prevent shell from displaying a mute bar
	audio_info->muted = config.muted;
}

static void SceShellMain_hang_exit(int device) {
	sceKernelUnlockMutex(top_func_exit_mtx, 1);
	load_config(device);
}

static int jav(SceSize argc, void *argv) { (void)argc; (void)argv;
	while (!audio_info) { sceKernelDelayThread(50 * 1000); }
	if (read_config() < 0) { reset_config(); }

	// initialise from config
	int old_device = get_device();
	sceKernelLockMutex(top_func_enter_mtx, 1, NULL);
	SceShellMain_hang_enter();
	SceShellMain_hang_exit(old_device);
	sceKernelUnlockMutex(top_func_enter_mtx, 1);

	// config is written to file after 3 seconds of no change
	jav_config_t old_config;
	sceClibMemcpy(&old_config, &config, sizeof(old_config));
	SceInt64 config_changed = sceKernelGetSystemTimeWide();

	while (run_thread) {
		LOG_FLUSH();
		sceKernelDelayThread(100 * 1000);
		disable_avls_timer();

		// persist config
		config.avls = get_avls();
		config.ob_volume[old_device] = get_ob_volume();
		config.muted = get_muted();
		if (sceClibMemcmp(&old_config, &config, sizeof(old_config)) != 0) {
			sceClibMemcpy(&old_config, &config, sizeof(old_config));
			config_changed = sceKernelGetSystemTimeWide();
		} else if (sceKernelGetSystemTimeWide() - config_changed >= 3 * 1000 * 1000) {
			write_config();
		}

		int device = get_device();
		if (device < 0) { continue; }

		if (old_device != device) {
			// apply automatic mute
			int speaker_mute = get_speaker_mute();
			if (device == SPEAKER && speaker_mute) { config.muted = 1; }

			sceKernelLockMutex(top_func_enter_mtx, 1, NULL);
			SceShellMain_hang_enter();
			init_vol_bar(&audio_info->vol_bar, VOL_BAR_INIT_MODE_INIT);
			if (config.muted) {
				set_vol_bar_muted(&audio_info->vol_bar, config.avls);
				SceShellMain_hang_exit(device);

				// need to try many times to ensure mute
				for (int i = 0; i < 15; i++) {
					sceKernelDelayThread(100 * 1000);
					mute_on();
				}
			} else {
				int old_vol = config.ob_volume[old_device];
				int new_vol = config.ob_volume[device];
				int flags = config.avls ? VOL_BAR_FLAG_AVLS : 0;
				set_vol_bar_lvl(&audio_info->vol_bar, old_vol, flags);
				SceShellMain_hang_exit(device);

				sceKernelDelayThread(400 * 1000);
				progress_vol_bar(old_vol, new_vol, flags);
				sceKernelDelayThread(800 * 1000);
			}
			free_vol_bar(&audio_info->vol_bar);
			sceKernelUnlockMutex(top_func_enter_mtx, 1);
			old_device = device;
		}
	}
	return 0;
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;
	LOG("\njav module starting\n");

	// create mutexes
	top_func_enter_mtx = sceKernelCreateMutex("jav_top_func_enter", 0, 0, NULL);
	top_func_exit_mtx = sceKernelCreateMutex("jav_top_func_exit", 0, 0, NULL);
	if (top_func_enter_mtx < 0 || top_func_exit_mtx < 0) {
		LOG("mutexes failed\n");
		goto exit;
	}

	// get module info
	tai_module_info_t mod_info;
	mod_info.size = sizeof(mod_info);
	if (taiGetModuleInfo("SceShell", &mod_info) < 0) {
		LOG("taiGetModuleInfo failed\n");
		goto exit;
	}

	// determine offsets
	int offset[N_HOOKS];
	switch (mod_info.module_nid) {
		case 0x0552F692:
			LOG("firmware 3.60 retail\n");
			offset[0] = 0x145422; offset[1] = 0x145C86; offset[2] = 0x146374;
			offset[3] = 0x147054; offset[4] = 0x145C5C;
			break;
		case 0x5549BF1F:
			LOG("firmware 3.65 retail\n");
fw365:		offset[0] = 0x14547A; offset[1] = 0x145CDE; offset[2] = 0x1463CC;
			offset[3] = 0x1470AC; offset[4] = 0x145CB4;
			break;
		case 0x34B4D82E:
			LOG("firmware 3.67 retail\n");
			goto fw365;
		case 0x12DAC0F3:
			LOG("firmware 3.68 retail\n");
			goto fw365;
		case 0x0703C828:
			LOG("firmware 3.69 retail\n");
			goto fw365;
		case 0x2053B5A5:
			LOG("firmware 3.70 retail\n");
			goto fw365;
		case 0xF476E785:
			LOG("firmware 3.71 retail\n");
			goto fw365;
		case 0x939FFBE9:
			LOG("firmware 3.72 retail\n");
			goto fw365;
		case 0x734D476A:
			LOG("firmware 3.73 retail\n");
			goto fw365;
		default:
			LOG("firmware unsupported\n");
			goto exit;
	}

	// setup hooks
	int hooks_succeed = 1;
	void *hook[N_HOOKS] = {top_func, init_vol_bar, set_vol_bar_lvl, set_vol_bar_muted, free_vol_bar};
	for (int i = 0; i < N_HOOKS; i++) {
		hook_id[i] = taiHookFunctionOffset(hook_ref+i, mod_info.modid, 0, offset[i], 1, hook[i]);
		if (hook_id[i] < 0) { hooks_succeed = 0; }
		LOG("hook %d: %08X\n", i, hook_id[i]);
	}
	if (!hooks_succeed) {
		LOG("hooks failed\n");
		goto exit;
	}

	// start thread
	thread_id = sceKernelCreateThread("jav", jav, 0x4C, 0x2000, 0, 0, NULL);
	if (thread_id < 0) {
		LOG("sceKernelCreateThread failed\n");
		goto exit;
	}
	if (sceKernelStartThread(thread_id, 0, NULL) < 0) {
		LOG("sceKernelStartThread failed\n");
		goto exit;
	}

exit:
	LOG_FLUSH();
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;
	LOG("jav module stopping\n");

	// destroy mutexes
	if (top_func_enter_mtx >= 0) { sceKernelDeleteMutex(top_func_enter_mtx); }
	if (top_func_exit_mtx >= 0) { sceKernelDeleteMutex(top_func_exit_mtx); }

	// stop thread
	if (thread_id >= 0) {
		run_thread = 0;
		sceKernelWaitThreadEnd(thread_id, NULL, NULL);
		sceKernelDeleteThread(thread_id);
		LOG("jav thread stopped\n");
	}

	// release hooks
	for (int i = 0; i < N_HOOKS; i++) {
		if (hook_id[i] >= 0) { taiHookRelease(hook_id[i], hook_ref[i]); }
	}

	LOG_FLUSH();
	return SCE_KERNEL_STOP_SUCCESS;
}
