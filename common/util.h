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

#ifndef UTIL_H
#define UTIL_H

#define CONFIG_BASE_DIR "ux0:data/"
#define CONFIG_JAV_DIR  CONFIG_BASE_DIR "jav/"
#define CONFIG_PATH     CONFIG_JAV_DIR "config.bin"
#define JAV_KERNEL_PATH CONFIG_JAV_DIR "jav_kernel.skprx"

#define USED __attribute__ ((used))
#define UNUSED __attribute__ ((unused))

#define OB2BT(x) (((x) * 127 + 15) / 30)
#define BT2OB(x) (((x) * 30 + 63) / 127)

#define RLZ(x) do {\
	int __ret__ = (x);\
	if (__ret__ < 0) {\
		LOG(#x " %08X\n", __ret__);\
		return __ret__;\
	}\
} while(0)

#define RNE(x, k) do {\
	int __ret__ = (x);\
	if (__ret__ != (k)) {\
		LOG(#x " %08X\n", __ret__);\
		return -1;\
	}\
} while(0)

#define GLZ(x) do {\
	if ((x) < 0) { goto fail; }\
} while(0)

int decode_bl_t1(int bl, int *imm);

#endif
