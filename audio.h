#ifndef AUDIO_H
#define AUDIO_H

#define AVLS_MAX 0x15

int get_device(void);
#define N_DEVICE_ONBOARD 0x2
#define SPEAKER   0
#define HEADPHONE 1

int get_ob_volume(void);
int set_ob_volume(int vol);

int get_muted(void);
int mute_on(void);

int get_avls(void);
int set_avls(int v);

int disable_avls_timer(void);

int get_speaker_mute(void);
int set_speaker_mute(int v);

#endif
