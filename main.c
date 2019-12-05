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

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>
#include <taihen.h>
#include "sce_shell.h"
#include "audio.h"
#include "config.h"
#include "util.h"
#include "log.h"

extern int ScePafToplevel_004D98CC(char *r1);
extern int ScePafToplevel_1DF2C6FD(int r1, int r2);

extern char jav_kernel_skprx[];
extern int jav_kernel_skprx_len;
static SceUID javk_id = -1;

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

static int set_mute_icon(int device, int muted) {
	if (sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_PAF) < 0) {
		return -1;
	}
	int v = device == BLUETOOTH ? 0 : muted;
	int a = ScePafToplevel_004D98CC("indicator_plugin");
	if (a != 0 && (a = ScePafToplevel_1DF2C6FD(a, 1)) != 0) {
		(*(set_mute_icon_ptr*)(a + 0x20))(v);
		return 0;
	} else {
		return -1;
	}
}

static void SceShellMain_hang_exit(void) {
	sceKernelUnlockMutex(top_func_exit_mtx, 1);
}

static int SceShellMain_hang_enter(int device, int mac0, int mac1, int muted) {
	sceKernelLockMutex(top_func_exit_mtx, 1, NULL);
	SceKernelMutexInfo info;
	while ((info.size = sizeof(info), sceKernelGetMutexInfo(top_func_exit_mtx, &info)) < 0
			|| info.numWaitThreads == 0) {
		sceKernelDelayThread(10 * 1000);
	}

	int mac0_, mac1_;
	if (get_device(&mac0_, &mac1_) == device && mac0_ == mac0 && mac1_ == mac1) {
		int new_vol = load_config(device, mac0, mac1);
		if (new_vol >= 0 && set_mute_icon(device, muted) == 0) {
			// keep SceShell state consistent to prevent extra mute bar
			// from displaying and wrong volume from being set
			audio_info->muted = muted;
			audio_info->volume = new_vol;
			return new_vol;
		}
	}
	SceShellMain_hang_exit();
	return -1;
}

static int switch_audio(int device, int mac0, int mac1, int avls, int muted, int old_vol) {
	sceKernelLockMutex(top_func_enter_mtx, 1, NULL);
	int new_vol = SceShellMain_hang_enter(device, mac0, mac1, muted);

	if (new_vol >= 0) {
		init_vol_bar(&audio_info->vol_bar, VOL_BAR_INIT_MODE_INIT);
		if (muted && device != BLUETOOTH) {
			set_vol_bar_muted(&audio_info->vol_bar, avls);
			SceShellMain_hang_exit();

			// need to try many times to ensure mute
			for (int i = 0; i < 15; i++) {
				sceKernelDelayThread(100 * 1000);
				mute_on();
			}
		} else {
			int flags = (avls ? VOL_BAR_FLAG_AVLS : 0) | (muted ? VOL_BAR_FLAG_MUTED : 0);
			if (device == BLUETOOTH) {
				flags = (flags & ~VOL_BAR_FLAG_AVLS) | VOL_BAR_FLAG_BT;
			}
			set_vol_bar_lvl(&audio_info->vol_bar, old_vol, flags);
			SceShellMain_hang_exit();

			sceKernelDelayThread(400 * 1000);
			progress_vol_bar(old_vol, new_vol, flags);
			sceKernelDelayThread(800 * 1000);
		}
		free_vol_bar(&audio_info->vol_bar);
	}

	sceKernelUnlockMutex(top_func_enter_mtx, 1);
	return new_vol;
}

static int jav(SceSize argc, void *argv) { (void)argc; (void)argv;
	while (!audio_info) { sceKernelDelayThread(50 * 1000); }
	if (read_config() < 0) { reset_config(); }

	// config is written to file after 3 seconds of no change
	jav_config_t old_config;
	sceClibMemcpy(&old_config, &config, sizeof(old_config));
	SceInt64 config_changed = sceKernelGetSystemTimeWide();

	int old_device = -1, old_mac0 = 0, old_mac1 = 0, old_vol = 0;

	while (run_thread) {
		LOG_FLUSH();
		sceKernelDelayThread(100 * 1000);
		disable_avls_timer();

		int mac0, mac1;
		int device = get_device(&mac0, &mac1);
		if (device < 0) { continue; }

		if (old_device != device || old_mac0 != mac0 || old_mac1 != mac1) {
			int speaker_mute = get_speaker_mute();
			if (speaker_mute >= 0) {
				if (old_device == HEADPHONE && device == SPEAKER && speaker_mute) { config.muted = 1; }
				// this is default behaviour
				if (old_device == BLUETOOTH && device != BLUETOOTH) { config.muted = 1; }

				int new_vol = switch_audio(device, mac0, mac1, config.avls, config.muted, old_vol);
				if (new_vol >= 0) {
					old_device = device;
					old_mac0 = mac0;
					old_mac1 = mac1;
					old_vol = new_vol;
					continue;
				}
			}

			old_device = -1;
			old_mac0 = 0;
			old_mac1 = 0;
			continue;
		}

		// persist config
		int new_vol = save_config(device, mac0, mac1);
		if (sceClibMemcmp(&old_config, &config, sizeof(old_config)) != 0) {
			sceClibMemcpy(&old_config, &config, sizeof(old_config));
			config_changed = sceKernelGetSystemTimeWide();
		} else if (sceKernelGetSystemTimeWide() - config_changed >= 3 * 1000 * 1000) {
			write_config();
		}

		if (new_vol >= 0) { old_vol = new_vol; }
	}
	return 0;
}

static void cleanup(void) {
	if (thread_id >= 0) {
		run_thread = 0;
		sceKernelWaitThreadEnd(thread_id, NULL, NULL);
		sceKernelDeleteThread(thread_id);
		LOG("JAVMain stopped and deleted\n");
	}

	for (int i = 0; i < N_HOOKS; i++) {
		if (hook_id[i] >= 0) {
			taiHookRelease(hook_id[i], hook_ref[i]);
			LOG("hook %d released\n", i);
		}
	}

	if (top_func_enter_mtx >= 0) {
		sceKernelDeleteMutex(top_func_enter_mtx);
		LOG("JAVTopFuncEnterMtx deleted\n");
	}
	if (top_func_exit_mtx >= 0) {
		sceKernelDeleteMutex(top_func_exit_mtx);
		LOG("JAVTopFuncExitMtx deleted\n");
	}

	if (javk_id >= 0) {
		taiStopUnloadKernelModule(javk_id, 0, NULL, 0, NULL, NULL);
		LOG("JAVKernel stopped and unloaded\n");
	}

	LOG("JAV cleanup complete\n");
}

static int extract_jav_kernel(void) {
	sceIoMkdir(CONFIG_BASE_DIR, 0777);
	sceIoMkdir(CONFIG_JAV_DIR, 0777);
	SceUID fd = sceIoOpen(JAV_KERNEL_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	RLZ(fd);

	int ret = sceIoWrite(fd, jav_kernel_skprx, jav_kernel_skprx_len);
	sceIoClose(fd);
	if (ret == jav_kernel_skprx_len) {
		LOG("%s extraction complete\n", JAV_KERNEL_PATH);
		return 0;
	} else {
		LOG("%s extraction failed\n", JAV_KERNEL_PATH);
		return -1;
	}
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;
	LOG("JAV starting\n");

	// extract and load JAVKernel
	GLZ(extract_jav_kernel());
	javk_id = taiLoadStartKernelModule(JAV_KERNEL_PATH, 0, NULL, 0);
	GLZ(javk_id);
	LOG("JAVKernel loaded and started\n");

	// create mutexes
	top_func_enter_mtx = sceKernelCreateMutex("JAVTopFuncEnterMtx", 0, 0, NULL);
	GLZ(top_func_enter_mtx);
	top_func_exit_mtx = sceKernelCreateMutex("JAVTopFuncExitMtx", 0, 0, NULL);
	GLZ(top_func_exit_mtx);
	LOG("JAV mutexes created\n");

	// get module info
	tai_module_info_t mod_info;
	mod_info.size = sizeof(mod_info);
	GLZ(taiGetModuleInfo("SceShell", &mod_info));
	LOG("SceShell module info acquired\n");

	// determine offsets
	int offset[N_HOOKS];
	switch (mod_info.module_nid) {
		case 0x0552F692: // 3.60 retail
			LOG("firmware 3.60 retail\n");
			offset[0] = 0x145422; offset[1] = 0x145C86; offset[2] = 0x146374;
			offset[3] = 0x147054; offset[4] = 0x145C5C;
			break;
		case 0x5549BF1F: // 3.65 retail
		case 0x34B4D82E: // 3.67 retail
		case 0x12DAC0F3: // 3.68 retail
		case 0x0703C828: // 3.69 retail
		case 0x2053B5A5: // 3.70 retail
		case 0xF476E785: // 3.71 retail
		case 0x939FFBE9: // 3.72 retail
		case 0x734D476A: // 3.73 retail
			LOG("firmware 3.65-3.73 retail\n");
			offset[0] = 0x14547A; offset[1] = 0x145CDE; offset[2] = 0x1463CC;
			offset[3] = 0x1470AC; offset[4] = 0x145CB4;
			break;
		default:
			LOG("firmware unsupported\n");
			goto fail;
	}

	// setup hooks
	void *hook[N_HOOKS] = {top_func, init_vol_bar, set_vol_bar_lvl, set_vol_bar_muted, free_vol_bar};
	for (int i = 0; i < N_HOOKS; i++) {
		hook_id[i] = taiHookFunctionOffset(hook_ref+i, mod_info.modid, 0, offset[i], 1, hook[i]);
		GLZ(hook_id[i]);
		LOG("hook %d: %08X\n", i, hook_id[i]);
	}

	// start thread (same priority as SceShellMain)
	thread_id = sceKernelCreateThread("JAVMain", jav, 0x4C, 0x3000, 0, 0, NULL);
	GLZ(thread_id);
	LOG("JAVMain created\n");
	GLZ(sceKernelStartThread(thread_id, 0, NULL));
	LOG("JAVMain started\n");

	LOG("JAV start success\n");
	return SCE_KERNEL_START_SUCCESS;

fail:
	LOG("JAV start failed\n");
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;
	LOG("JAV stopping\n");
	cleanup();
	LOG("JAV stop success\n");
	return SCE_KERNEL_STOP_SUCCESS;
}
