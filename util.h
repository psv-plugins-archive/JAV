#ifndef UTIL_H
#define UTIL_H

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

#endif
