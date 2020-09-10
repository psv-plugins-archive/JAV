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

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>
#include "util.h"

extern int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
#define GET_OFFSET(modid, seg, ofs, addr)\
	module_get_offset(KERNEL_PID, modid, seg, ofs, (uintptr_t*)addr)

#define HOOK_EXPORT(idx, mod, libnid, funcnid, func)\
	(hook_id[idx] = taiHookFunctionExportForKernel(\
		KERNEL_PID, hook_ref+idx, mod, libnid, funcnid, func##_hook))

#define INJECT_DATA(idx, modid, seg, ofs, src, size)\
	(inject_id[idx] = taiInjectDataForKernel(\
		KERNEL_PID, modid, seg, ofs, src, size))

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

USED int module_start(UNUSED SceSize args, UNUSED const void *argp) {
	tai_module_info_t mod_info;
	mod_info.size = sizeof(tai_module_info_t);

	GLZ(taiGetModuleInfoForKernel(KERNEL_PID, "SceAVConfig", &mod_info));
	SceUID mid = mod_info.modid;
	GLZ(GET_OFFSET(mid, 1, 0x2AC, &avconfig_intr_mtx));
	GLZ(GET_OFFSET(mid, 1, 0x278, &avconfig_bt_vol));

	GLZ(HOOK_EXPORT(0, "SceBt", 0x9785DB68, 0xC5C7003B, sceBtAvrcpSendVolume));

	// disable auto mute (jav will manage it)

	// when bluetooth device disconnects (nop nop)
	GLZ(INJECT_DATA(0, mid, 0, 0xF70, "\x00\xBF\x00\xBF", 4));

	// when speaker_mute is on and headphone is unplugged (b.w #-1552)
	GLZ(INJECT_DATA(1, mid, 0, 0xF88, "\xFF\xF7\xF6\xBC", 4));

	// disable forced AVLS
	int *avconfig_force_avls;
	GLZ(GET_OFFSET(mid, 1, 0x114, &avconfig_force_avls));
	*avconfig_force_avls = 0;

	return SCE_KERNEL_START_SUCCESS;

fail:
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

USED int module_stop(UNUSED SceSize args, UNUSED const void *argp) {
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
