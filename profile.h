#ifndef PROFILE_H
#define PROFILE_H

#include "volume.h"

typedef struct {
	int avls;
	int muted;
	int volumes[N_OUTPUTS];
} audio_profile_t;

extern audio_profile_t profile;

void reset_profile(void);
int load_profile(void);
int write_profile(void);
void apply_profile(int output);

#endif
