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
#include "sce_kernel.h"
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

static SceKernelLwMutexWork top_func_mtx __attribute__ ((aligned (8)));

static SceUID jav_evf = -1;
#define JAV_EVF_JAVMAIN_RUN          0x01
#define JAV_EVF_SCESHELLMAIN_RUN     0x10
#define JAV_EVF_SCESHELLMAIN_WAITING 0x20

#define N_OFFSET_FUNC 4
static int (*init_vol_bar)(volume_bar_t *v, int mode);
static int (*set_vol_bar_lvl)(volume_bar_t *v, int level, int flags);
static int (*set_vol_bar_muted)(volume_bar_t *v, int avls);
static int (*free_vol_bar)(volume_bar_t *v);

static SceUID         SceShell_id = -1;
static int            top_func_offset = -1;
static SceUID         top_func_hook_id = -1;
static tai_hook_ref_t top_func_hook_ref = -1;
#define HOOK_TOP_FUNC(x) (top_func_hook_id = taiHookFunctionOffset(&top_func_hook_ref, SceShell_id, 0, top_func_offset, 1, (x)))

static SceUID thread_id = -1;
static SceUID jav_timer = -1;

static void reset_jav_timer(SceKernelSysClock delay) {
	sceKernelStopTimer(jav_timer);
	sceKernelSetTimerTimeWide(jav_timer, 0);
	sceKernelClearEvent(jav_timer, ~SCE_KERNEL_EVENT_TIMER);
	sceKernelSetTimerEvent(jav_timer, SCE_KERNEL_TIMER_TYPE_PULSE_EVENT, &delay, 1);
	sceKernelStartTimer(jav_timer);
}

static void wait_jav_timer(void) {
	sceKernelWaitEvent(jav_timer, SCE_KERNEL_EVENT_TIMER, NULL, NULL, NULL);
}

static int top_func_hook(audio_info_t *a, button_info_t *p) {
	if (sceKernelTryLockLwMutex(&top_func_mtx, 1) == 0) {
		int ret = TAI_CONTINUE(int, top_func_hook_ref, a, p);
		sceKernelUnlockLwMutex(&top_func_mtx, 1);
		return ret;
	}

	if (sceKernelPollEventFlag(jav_evf, JAV_EVF_SCESHELLMAIN_RUN, SCE_KERNEL_EVF_WAITMODE_OR, NULL) < 0) {
		sceKernelSetEventFlag(jav_evf, JAV_EVF_SCESHELLMAIN_WAITING);
		sceKernelWaitEventFlag(jav_evf, JAV_EVF_SCESHELLMAIN_RUN, SCE_KERNEL_EVF_WAITMODE_OR, NULL, NULL);
	}
	return 0;
}

static void progress_vol_bar(audio_info_t *audio_info, int start, int end, int flag) {
	int steps = (start < end ? end - start : start - end) + 1;
	int delay = 800 * 1000 / steps;
	reset_jav_timer(delay);
	if (start < end) {
		for (int i = start; i <= end; i++) {
			set_vol_bar_lvl(&audio_info->vol_bar, i, flag);
			wait_jav_timer();
		}
	} else {
		for (int i = start; i >= end; i--) {
			set_vol_bar_lvl(&audio_info->vol_bar, i, flag);
			wait_jav_timer();
		}
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

static void SceShellMain_run(void) {
	sceKernelSetEventFlag(jav_evf, JAV_EVF_SCESHELLMAIN_RUN);
}

static int SceShellMain_wait(jav_config_t *config, int device, int mac0, int mac1, audio_info_t *audio_info) {
	sceKernelClearEventFlag(jav_evf, ~JAV_EVF_SCESHELLMAIN_RUN);
	sceKernelWaitEventFlag(jav_evf,
		JAV_EVF_SCESHELLMAIN_WAITING,
		SCE_KERNEL_EVF_WAITMODE_OR | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT,
		NULL,
		NULL);

	int mac0_, mac1_;
	if (get_device(&mac0_, &mac1_) == device && mac0_ == mac0 && mac1_ == mac1) {
		int new_vol = load_config(config, device, mac0, mac1);
		if (new_vol >= 0 && set_mute_icon(device, config->muted) == 0) {
			// keep SceShell state consistent to prevent extra mute bar
			// from displaying and wrong volume from being set
			audio_info->muted = config->muted;
			audio_info->volume = new_vol;
			return new_vol;
		}
	}
	SceShellMain_run();
	return -1;
}

static int switch_audio(jav_config_t *config, int device, int mac0, int mac1, audio_info_t *audio_info, int old_vol) {
	sceKernelLockLwMutex(&top_func_mtx, 1, NULL);
	int new_vol = SceShellMain_wait(config, device, mac0, mac1, audio_info);

	if (new_vol >= 0) {
		init_vol_bar(&audio_info->vol_bar, VOL_BAR_INIT_MODE_INIT);
		if (config->muted && device != BLUETOOTH) {
			set_vol_bar_muted(&audio_info->vol_bar, config->avls);
			SceShellMain_run();

			// need to try many times to ensure mute
			reset_jav_timer(100 * 1000);
			for (int i = 0; i < 16; i++) {
				wait_jav_timer();
				mute_on();
			}
		} else {
			int flags = (config->avls ? VOL_BAR_FLAG_AVLS : 0) | (config->muted ? VOL_BAR_FLAG_MUTED : 0);
			if (device == BLUETOOTH) {
				flags = (flags & ~VOL_BAR_FLAG_AVLS) | VOL_BAR_FLAG_BT;
			}
			set_vol_bar_lvl(&audio_info->vol_bar, old_vol, flags);
			SceShellMain_run();

			progress_vol_bar(audio_info, old_vol, new_vol, flags);
			reset_jav_timer(800 * 1000);
			wait_jav_timer();
		}
		free_vol_bar(&audio_info->vol_bar);
	}

	sceKernelUnlockLwMutex(&top_func_mtx, 1);
	return new_vol;
}

static int jav(SceSize argc, void *argv) { (void)argc;
	audio_info_t *audio_info = *(audio_info_t**)argv;

	// config is written to file after 3 seconds of no change
	jav_config_t config;
	if (read_config(&config) < 0) { reset_config(&config); }
	jav_config_t old_config;
	sceClibMemcpy(&old_config, &config, sizeof(old_config));
	SceInt64 config_changed = sceKernelGetSystemTimeWide();

	int old_device = -1, old_mac0 = 0, old_mac1 = 0, old_vol = 0;
	reset_jav_timer(100 * 1000);

	while (sceKernelPollEventFlag(jav_evf, JAV_EVF_JAVMAIN_RUN, SCE_KERNEL_EVF_WAITMODE_OR, NULL) == 0) {
		wait_jav_timer();
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

				int new_vol = switch_audio(&config, device, mac0, mac1, audio_info, old_vol);
				if (new_vol >= 0) {
					old_device = device;
					old_mac0 = mac0;
					old_mac1 = mac1;
					old_vol = new_vol;
					reset_jav_timer(100 * 1000);
					continue;
				}
			}

			old_device = -1;
			old_mac0 = 0;
			old_mac1 = 0;
			reset_jav_timer(50 * 1000);
			continue;
		}

		// persist config
		int new_vol = save_config(&config, device, mac0, mac1);
		if (sceClibMemcmp(&old_config, &config, sizeof(old_config)) != 0) {
			sceClibMemcpy(&old_config, &config, sizeof(old_config));
			config_changed = sceKernelGetSystemTimeWide();
		} else if (sceKernelGetSystemTimeWide() - config_changed >= 3 * 1000 * 1000) {
			write_config(&config);
		}

		if (new_vol >= 0) { old_vol = new_vol; }
	}
	return 0;
}

static int jav_thread_launcher(audio_info_t *a, button_info_t *p) {
	int ret = TAI_CONTINUE(int, top_func_hook_ref, a, p);

	// rehook top_func
	GLZ(taiHookRelease(top_func_hook_id, top_func_hook_ref));
	GLZ(HOOK_TOP_FUNC(top_func_hook));
	GLZ(top_func_hook_id);
	LOG("top_func rehooked\n");

	// launch jav thread
	thread_id = sceKernelCreateThread("JAVMain", jav,
		SCE_KERNEL_CURRENT_THREAD_PRIORITY,
		0x3000,
		0,
		sceKernelGetThreadCpuAffinityMask(SCE_KERNEL_THREAD_ID_SELF),
		NULL);
	GLZ(thread_id);
	LOG("JAVMain created\n");
	GLZ(sceKernelStartThread(thread_id, sizeof(a), &a));
	LOG("JAVMain started\n");

fail:
	return ret;
}

static void cleanup(void) {
	if (thread_id >= 0) {
		sceKernelClearEventFlag(jav_evf, ~JAV_EVF_JAVMAIN_RUN);
		sceKernelWaitThreadEnd(thread_id, NULL, NULL);
		sceKernelDeleteThread(thread_id);
		LOG("JAVMain stopped and deleted\n");
	}

	if (top_func_hook_id >= 0) {
		taiHookRelease(top_func_hook_id, top_func_hook_ref);
		LOG("top_func_hook released\n", top_func_hook_id);
	}

	sceKernelDeleteLwMutex(&top_func_mtx);
	LOG("JAVTopFuncMtx deleted\n");

	if (jav_evf >= 0) {
		sceKernelDeleteEventFlag(jav_evf);
		LOG("JAVEventFlag deleted\n");
	}

	if (jav_timer >= 0) {
		sceKernelDeleteTimer(jav_timer);
		LOG("JAVTimer deleted\n");
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

	// create mutex
	GLZ(sceKernelCreateLwMutex(&top_func_mtx, "JAVTopFuncMtx", SCE_KERNEL_MUTEX_ATTR_TH_FIFO, 0, NULL));
	LOG("JAVTopFuncMtx created\n");

	// create event flag
	jav_evf = sceKernelCreateEventFlag("JAVEventFlag",
		SCE_KERNEL_EVF_ATTR_TH_PRIO | SCE_KERNEL_EVF_ATTR_MULTI,
		JAV_EVF_JAVMAIN_RUN | JAV_EVF_SCESHELLMAIN_RUN,
		NULL);
	GLZ(jav_evf);
	LOG("JAVEventFlag created\n");

	// create timer
	jav_timer = sceKernelCreateTimer("JavTimer",
		SCE_KERNEL_ATTR_TH_FIFO | SCE_KERNEL_EVENT_ATTR_AUTO_RESET | SCE_KERNEL_ATTR_NOTIFY_CB_WAKEUP_ONLY,
		NULL);
	GLZ(jav_timer);
	LOG("JAVTimer created\n");

	// get module info
	tai_module_info_t mod_info;
	mod_info.size = sizeof(mod_info);
	GLZ(taiGetModuleInfo("SceShell", &mod_info));
	SceShell_id = mod_info.modid;

	SceKernelModuleInfo sce_mod_info;
	sce_mod_info.size = sizeof(sce_mod_info);
	GLZ(sceKernelGetModuleInfo(SceShell_id, &sce_mod_info));
	LOG("SceShell module info acquired\n");

	// determine offsets
	int offset_func_offset[N_OFFSET_FUNC];
	switch (mod_info.module_nid) {
		case 0x0552F692: // 3.60 retail
			LOG("firmware 3.60 retail\n");
			offset_func_offset[0] = 0x145C86; offset_func_offset[1] = 0x146374;
			offset_func_offset[2] = 0x147054; offset_func_offset[3] = 0x145C5C;
			top_func_offset = 0x145422;
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
			offset_func_offset[0] = 0x145CDE; offset_func_offset[1] = 0x1463CC;
			offset_func_offset[2] = 0x1470AC; offset_func_offset[3] = 0x145CB4;
			top_func_offset = 0x14547A;
			break;
		default:
			LOG("firmware unsupported\n");
			goto fail;
	}

	// setup offset funcs
	int *offset_func[N_OFFSET_FUNC] = {
		(int*)&init_vol_bar,
		(int*)&set_vol_bar_lvl,
		(int*)&set_vol_bar_muted,
		(int*)&free_vol_bar};
	for (int i = 0; i < N_OFFSET_FUNC; i++) {
		*offset_func[i] = ((int)sce_mod_info.segments[0].vaddr + offset_func_offset[i]) | 1;
		LOG("Offset function %d: %08X\n", i, *(offset_func[i]));
	}

	// setup hooks
	GLZ(HOOK_TOP_FUNC(jav_thread_launcher));
	LOG("top_func_hook_id: %08X\n", top_func_hook_id);

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
