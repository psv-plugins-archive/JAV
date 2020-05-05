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

#ifndef SCE_KERNEL_H
#define SCE_KERNEL_H

#define SCE_KERNEL_ATTR_SINGLE  0x00000000
#define SCE_KERNEL_ATTR_MULTI   0x00001000
#define SCE_KERNEL_ATTR_TH_FIFO 0x00000000
#define SCE_KERNEL_ATTR_TH_PRIO 0x00002000

#define SCE_KERNEL_THREAD_ID_SELF 0
#define SCE_KERNEL_CURRENT_THREAD_PRIORITY 0

#define SCE_KERNEL_EVF_ATTR_TH_FIFO SCE_KERNEL_ATTR_TH_FIFO
#define SCE_KERNEL_EVF_ATTR_TH_PRIO SCE_KERNEL_ATTR_TH_PRIO
#define SCE_KERNEL_EVF_ATTR_SINGLE  SCE_KERNEL_ATTR_SINGLE
#define SCE_KERNEL_EVF_ATTR_MULTI   SCE_KERNEL_ATTR_MULTI

#define SCE_KERNEL_EVF_WAITMODE_AND       0x00000000
#define SCE_KERNEL_EVF_WAITMODE_OR        0x00000001
#define SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL 0x00000002
#define SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT 0x00000004

#define SCE_KERNEL_MUTEX_ATTR_TH_FIFO   SCE_KERNEL_ATTR_TH_FIFO
#define SCE_KERNEL_MUTEX_ATTR_TH_PRIO   SCE_KERNEL_ATTR_TH_PRIO
#define SCE_KERNEL_MUTEX_ATTR_RECURSIVE 0x00000002
#define SCE_KERNEL_MUTEX_ATTR_CEILING   0x00000004

#define SCE_KERNEL_EVENT_ATTR_MANUAL_RESET    0x00000000
#define SCE_KERNEL_EVENT_ATTR_AUTO_RESET      0x00000100
#define SCE_KERNEL_ATTR_NOTIFY_CB_ALL         0x00000000
#define SCE_KERNEL_ATTR_NOTIFY_CB_WAKEUP_ONLY 0x00000800

#define SCE_KERNEL_EVENT_IN                0x00000001
#define SCE_KERNEL_EVENT_OUT               0x00000002
#define SCE_KERNEL_EVENT_CREATE            0x00000004
#define SCE_KERNEL_EVENT_DELETE            0x00000008
#define SCE_KERNEL_EVENT_ERROR             0x00000010
#define SCE_KERNEL_EVENT_OPEN              0x00000100
#define SCE_KERNEL_EVENT_CLOSE             0x00000200
#define SCE_KERNEL_EVENT_TIMER             0x00008000
#define SCE_KERNEL_EVENT_DATA_EXIST        0x00010000
#define SCE_KERNEL_EVENT_USER_DEFINED_MASK 0xFF000000

#define SCE_KERNEL_TIMER_TYPE_SET_EVENT   0
#define SCE_KERNEL_TIMER_TYPE_PULSE_EVENT 1

typedef struct _SceKernelTimerOptParam {
	SceSize size;
} SceKernelTimerOptParam;

SceInt32 sceKernelGetThreadCpuAffinityMask(SceUID threadId);

SceInt32 sceKernelWaitEvent(SceUID eventId, SceUInt32 waitPattern, SceUInt32 *pResultPattern, SceUInt64 *pUserData, SceUInt32 *pTimeout);
SceInt32 sceKernelClearEvent(SceUID eventId, SceUInt32 clearPattern);

SceUID sceKernelCreateTimer(const char *pName, SceUInt32 attr, const SceKernelTimerOptParam *pOptParam);
SceInt32 sceKernelDeleteTimer(SceUID timerId);
SceInt32 sceKernelSetTimerEvent(SceUID timerId, SceInt32 type, SceKernelSysClock *pInterval, SceInt32 fRepeat);
SceInt32 sceKernelStartTimer(SceUID timerId);
SceInt32 sceKernelStopTimer(SceUID timerId);
SceUInt64 sceKernelSetTimerTimeWide(SceUID timerId, SceUInt64 clock);

#endif
