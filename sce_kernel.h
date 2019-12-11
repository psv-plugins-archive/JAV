#ifndef SCE_KERNEL_H
#define SCE_KERNEL_H

#define SCE_KERNEL_ATTR_SINGLE  0x00000000
#define SCE_KERNEL_ATTR_MULTI   0x00001000
#define SCE_KERNEL_ATTR_TH_FIFO 0x00000000
#define SCE_KERNEL_ATTR_TH_PRIO 0x00002000

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

#endif
