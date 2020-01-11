/*
自動オーディオボリューム
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

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>
#include "util.h"

// taihenModuleUtils_stub
extern int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

#define N_HOOK 1
static SceUID hook_id[N_HOOK];
static tai_hook_ref_t hook_ref[N_HOOK];

#define N_INJECT 2
static SceUID inject_id[N_INJECT];

static int *avconfig_intr_mtx;
static int *avconfig_bt_vol;

static int sceBtAvrcpSendVolume_hook(int mac0, int mac1, int vol) {
	int ret = TAI_CONTINUE(int, hook_ref[0], mac0, mac1, vol);
	if (ret >= 0) {
		int syscall_state, intr_state;
		ENTER_SYSCALL(syscall_state);
		intr_state = ksceKernelCpuSuspendIntr(avconfig_intr_mtx);
		*avconfig_bt_vol = BT2OB(vol);
		ksceKernelCpuResumeIntr(avconfig_intr_mtx, intr_state);
		EXIT_SYSCALL(syscall_state);
	}
	return ret;
}

static void cleanup(void) {
	for (int i = 0; i < N_HOOK; i++) {
		if (hook_id[i] >= 0) { taiHookReleaseForKernel(hook_id[i], hook_ref[i]); }
	}

	for (int i = 0; i < N_INJECT; i++) {
		if (inject_id[i] >= 0) { taiInjectReleaseForKernel(inject_id[i]); }
	}
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;
	tai_module_info_t mod_info;
	mod_info.size = sizeof(tai_module_info_t);

	GLZ(taiGetModuleInfoForKernel(KERNEL_PID, "SceAVConfig", &mod_info));
	GLZ(module_get_offset(KERNEL_PID, mod_info.modid, 1, 0x2AC, (uintptr_t*)&avconfig_intr_mtx));
	GLZ(module_get_offset(KERNEL_PID, mod_info.modid, 1, 0x278, (uintptr_t*)&avconfig_bt_vol));

	hook_id[0] = taiHookFunctionExportForKernel(KERNEL_PID, hook_ref+0, "SceBt", 0x9785DB68, 0xC5C7003B, sceBtAvrcpSendVolume_hook);
	GLZ(hook_id[0]);

	// disable auto mute (jav will manage it)

	// when bluetooth device disconnects
	inject_id[0] = taiInjectDataForKernel(KERNEL_PID, mod_info.modid, 0, 0xF70, "\x00\xBF\x00\xBF", 4); // nop nop
	GLZ(inject_id[0]);

	// when speaker_mute is on and headphone is unplugged
	inject_id[1] = taiInjectDataForKernel(KERNEL_PID, mod_info.modid, 0, 0xF88, "\xFF\xF7\xF6\xBC", 4); // b.w -1552
	GLZ(inject_id[1]);

	// disable forced AVLS
	int *avconfig_force_avls;
	GLZ(module_get_offset(KERNEL_PID, mod_info.modid, 1, 0x114, (uintptr_t*)&avconfig_force_avls));
	*avconfig_force_avls = 0;

	return SCE_KERNEL_START_SUCCESS;

fail:
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
