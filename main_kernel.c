/*
自動オーディオボリューム - main_kernel.c
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

static SceUID hook_id;
static tai_hook_ref_t hook_ref;

static int *avconfig_intr_mtx;
static int *avconfig_bt_vol;

static int sceBtAvrcpSendVolume_hook(int mac0, int mac1, int vol) {
	int ret = TAI_CONTINUE(int, hook_ref, mac0, mac1, vol);
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
	if (hook_id >= 0) { taiHookReleaseForKernel(hook_id, hook_ref); }
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;
	tai_module_info_t mod_info;
	mod_info.size = sizeof(tai_module_info_t);

	GLZ(taiGetModuleInfoForKernel(KERNEL_PID, "SceAVConfig", &mod_info));
	GLZ(module_get_offset(KERNEL_PID, mod_info.modid, 1, 0x2AC, (uintptr_t*)&avconfig_intr_mtx));
	GLZ(module_get_offset(KERNEL_PID, mod_info.modid, 1, 0x278, (uintptr_t*)&avconfig_bt_vol));

	hook_id = taiHookFunctionExportForKernel(KERNEL_PID, &hook_ref, "SceBt", 0x9785DB68, 0xC5C7003B, sceBtAvrcpSendVolume_hook);
	GLZ(hook_id);

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
