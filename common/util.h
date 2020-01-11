#ifndef UTIL_H
#define UTIL_H

#define CONFIG_BASE_DIR "ux0:data/"
#define CONFIG_JAV_DIR  CONFIG_BASE_DIR "jav/"
#define CONFIG_PATH     CONFIG_JAV_DIR "config.bin"
#define JAV_KERNEL_PATH CONFIG_JAV_DIR "jav_kernel.skprx"

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

#endif
