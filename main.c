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
#include "volume.h"
#include "profile.h"
#include "log.h"

extern int ScePafToplevel_004D98CC(char *r1);
extern int ScePafToplevel_1DF2C6FD(int r1, int r2);

SceUID top_func_enter_mtx = -1;
SceUID top_func_exit_mtx = -1;

#define N_HOOKS 5
SceUID hook_id[N_HOOKS];
tai_hook_ref_t hook_ref[N_HOOKS];

void *vol_bar_ctx = 0;
volatile char *shell_mute_status = 0;
SceUID thread_id = -1;
int run_thread = 1;

int top_func(int r1, int r2) {
	if (!vol_bar_ctx) {
		vol_bar_ctx = (void*)(r1 + 0x20);
		// set this field to prevent shell from
		// displaying a second mute bar
		shell_mute_status = (char*)(r1 + 0x49);
	}

	if (sceKernelTryLockMutex(top_func_enter_mtx, 1) == 0) {
		int ret = TAI_CONTINUE(int, hook_ref[0], r1, r2);
		sceKernelUnlockMutex(top_func_enter_mtx, 1);
		return ret;
	}

	sceKernelLockMutex(top_func_exit_mtx, 1, 0);
	sceKernelUnlockMutex(top_func_exit_mtx, 1);
	return 0;
}

int init_vol_bar(void *r1, int r2) {
	return TAI_CONTINUE(int, hook_ref[1], r1, r2);
}

int set_vol_bar_lvl(void *r1, int r2, int r3) {
	return TAI_CONTINUE(int, hook_ref[2], r1, r2, r3);
}

int set_vol_bar_muted(void *r1, int r2) {
	return TAI_CONTINUE(int, hook_ref[3], r1, r2);
}

int free_vol_bar(void *r1) {
	return TAI_CONTINUE(int, hook_ref[4], r1);
}

void progress_vol_bar(int start, int end, int flag) {
	if (start < end) {
		for (int i = start; i <= end; i++) {
			set_vol_bar_lvl(vol_bar_ctx, i, flag);
			sceKernelDelayThread(20 * 1000);
		}
	} else if (start > end) {
		for (int i = start; i >= end; i--) {
			set_vol_bar_lvl(vol_bar_ctx, i, flag);
			sceKernelDelayThread(20 * 1000);
		}
	} else {
		set_vol_bar_lvl(vol_bar_ctx, start, flag);
	}
}

typedef int (*set_mute_icon_ptr)(int);

int set_mute_icon(int v) {
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

void SceShellMain_hang_enter(void) {
	for (;;) {
		sceKernelLockMutex(top_func_exit_mtx, 1, 0);
		SceKernelMutexInfo info;
		while ((info.size = sizeof(info), sceKernelGetMutexInfo(top_func_exit_mtx, &info)) < 0
				|| info.numWaitThreads == 0) {
			sceKernelDelayThread(10 * 1000);
		}
		if (set_mute_icon(profile.muted) == 0) {
			break;
		} else {
			sceKernelUnlockMutex(top_func_exit_mtx, 1);
			sceKernelDelayThread(50 * 1000);
		}
	}
	*shell_mute_status = profile.muted;
}

void SceShellMain_hang_exit(int output) {
	sceKernelUnlockMutex(top_func_exit_mtx, 1);
	apply_profile(output);
}

int jav() {
	while (!vol_bar_ctx || !shell_mute_status) {
		sceKernelDelayThread(50 * 1000);
	}
	if (load_profile() < 0) { reset_profile(); }

	// initialise from config
	int last_output = get_output();
	sceKernelLockMutex(top_func_enter_mtx, 1, 0);
	SceShellMain_hang_enter();
	SceShellMain_hang_exit(last_output);
	sceKernelUnlockMutex(top_func_enter_mtx, 1);

	// profile is written to file after 3 seconds of no change
	audio_profile_t profile_buffer;
	sceClibMemcpy(&profile_buffer, &profile, sizeof(profile_buffer));
	SceInt64 profile_last_changed = sceKernelGetSystemTimeWide();

	while (run_thread) {
		LOG_FLUSH();
		sceKernelDelayThread(100 * 1000);
		disable_avls_timer();

		// persist profile settings
		profile.avls = get_avls();
		profile.volumes[last_output] = get_volume();
		profile.muted = get_muted();
		if (sceClibMemcmp(&profile_buffer, &profile, sizeof(profile_buffer)) != 0) {
			sceClibMemcpy(&profile_buffer, &profile, sizeof(profile_buffer));
			profile_last_changed = sceKernelGetSystemTimeWide();
		} else if (sceKernelGetSystemTimeWide() - profile_last_changed >= 3 * 1000 * 1000) {
			write_profile();
		}

		int output = get_output();
		if (output < 0) { continue; }

		if (last_output != output) {
			int speaker_mute = get_speaker_mute();
			if (output == SPEAKER && speaker_mute) {
				LOG("speaker_mute applied\n");
				profile.muted = 1;
			}

			sceKernelLockMutex(top_func_enter_mtx, 1, 0);
			SceShellMain_hang_enter();
			init_vol_bar(vol_bar_ctx, 1);
			if (profile.muted) {
				set_vol_bar_muted(vol_bar_ctx, profile.avls);
				SceShellMain_hang_exit(output);

				// need to try many times to ensure mute
				for (int i = 0; i < 15; i++) {
					sceKernelDelayThread(100 * 1000);
					mute_on();
				}
			} else {
				int cur_vol = profile.volumes[last_output];
				int tgt_vol = profile.volumes[output];
				set_vol_bar_lvl(vol_bar_ctx, cur_vol, profile.avls << 2);
				SceShellMain_hang_exit(output);

				sceKernelDelayThread(400 * 1000);
				progress_vol_bar(cur_vol, tgt_vol, profile.avls << 2);
				sceKernelDelayThread(800 * 1000);
			}
			free_vol_bar(vol_bar_ctx);
			sceKernelUnlockMutex(top_func_enter_mtx, 1);
			last_output = output;
		}
	}
	return 0;
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start() {
	LOG("\njav module starting\n");

	// create mutexes
	top_func_enter_mtx = sceKernelCreateMutex("jav_top_func_enter", 0, 0, 0);
	top_func_exit_mtx = sceKernelCreateMutex("jav_top_func_exit", 0, 0, 0);
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
	thread_id = sceKernelCreateThread("jav", jav, 0x4C, 0x2000, 0, 0, 0);
	if (thread_id < 0) {
		LOG("sceKernelCreateThread failed\n");
		goto exit;
	}
	if (sceKernelStartThread(thread_id, 0, 0) < 0) {
		LOG("sceKernelStartThread failed\n");
		goto exit;
	}

exit:
	LOG_FLUSH();
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop() {
	LOG("jav module stopping\n");

	// destroy mutexes
	if (top_func_enter_mtx >= 0) { sceKernelDeleteMutex(top_func_enter_mtx); }
	if (top_func_exit_mtx >= 0) { sceKernelDeleteMutex(top_func_exit_mtx); }

	// stop thread
	if (thread_id >= 0) {
		run_thread = 0;
		sceKernelWaitThreadEnd(thread_id, 0, 0);
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
