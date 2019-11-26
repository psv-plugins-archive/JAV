#ifndef SCE_AVCONFIG_H
#define SCE_AVCONFIG_H

extern int sceAVConfigWriteRegSystemVol(int vol);

// vol_ctrl returns garbage when volume cannot be controlled
extern int sceAVConfigGetVolCtrlEnable(int *vol_ctrl, int *muted, int *avls);
#define AVCONFIG_VOLCTRL_UNK_0     0
#define AVCONFIG_VOLCTRL_ONBOARD   1
#define AVCONFIG_VOLCTRL_BLUETOOTH 2
#define AVCONFIG_VOLCTRL_UNK_3     3
#define AVCONFIG_VOLCTRL_UNK_4     4

extern int sceAVConfigGetConnectedAudioDevice(int *flags);
#define AVCONFIG_CONDEV_VITA_0       0x001 // not Dolce
#define AVCONFIG_CONDEV_MULTI_CN     0x002 // (ksceSysconGetMultiCnInfo & 0xff00) == 0x300 (from SceHpremoteForDriver_2229EF51)
#define AVCONFIG_CONDEV_AUDIO_OUT    0x004 // 3.5mm at least 3 pin
#define AVCONFIG_CONDEV_HDMI         0x008 // HDMI audio mode?
#define AVCONFIG_CONDEV_BT_AUDIO_OUT 0x010 // bluetooth audio out
#define AVCONFIG_CONDEV_VITA_8       0x100 // not Dolce
#define AVCONFIG_CONDEV_AUDIO_IN     0x400 // 3.5mm 4 pin with microphone
#define AVCONFIG_CONDEV_BT_AUDIO_IN  0x800 // bluetooth microphone

extern int sceAVConfigChangeReg(int k, int v);
#define AVCONFIG_REG_BT_MIC       0
#define AVCONFIG_REG_AVLS         1
#define AVCONFIG_REG_SOUND_OUTPUT 2
#define AVCONFIG_REG_AVLS_TIMER   3
#define AVCONFIG_REG_SPEAKER_MUTE 4

#endif
