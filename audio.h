#ifndef AUDIO_H
#define AUDIO_H

#define AVLS_MAX 0x15

int get_device(int *mac0, int *mac1);
#define N_DEVICE_ONBOARD   0x2
#define N_DEVICE_BLUETOOTH 0x20
#define SPEAKER   0
#define HEADPHONE 1
#define BLUETOOTH 2

int get_ob_volume(void);
int set_ob_volume(int vol);

int get_muted(void);
int mute_on(void);

int get_avls(void);
int set_avls(int v);

int disable_avls_timer(void);

int get_speaker_mute(void);
int set_speaker_mute(int v);

int get_bt_volume(int mac0, int mac1);
int set_bt_volume(int mac0, int mac1, int vol);

#endif
