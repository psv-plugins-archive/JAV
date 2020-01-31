# 自動オーディオボリューム

[**Download**](https://github.com/cuevavirus/jav/releases)

Automatically switch between speaker and headphone volumes.

![demo.gif](https://github.com/cuevavirus/jav/raw/assets/demo.gif)
![bt_demo.gif](https://github.com/cuevavirus/jav/raw/assets/bt_demo.gif)

## Installation

Supported firmware versions:

- Retail 3.60-3.73
- Testkit 3.65

Download the latest version from [releases](https://github.com/cuevavirus/jav/releases) and install under `*main` of your taiHEN config.

```
*main
ur0:tai/jav.suprx
```

## Usage

- When the audio output device is changed, the volume level automatically switches to the last used volume for that device.
- Bluetooth audio devices have volume levels saved per device, for up to 32 devices.
- When a Bluetooth audio device is disconnected, automatic mute follows your setting.
- If your Vita is muted, the volume changes but remains muted. Press the volume buttons to unmute.
- For Europe region Vitas, AVLS does not turn on automatically at boot or after 20 hours of playback.

## Credits

- Plugin idea: [nkekev](https://twitter.com/Nkekev)
- Testing: [dots-tb](https://www.youtube.com/channel/UCsGdCQOiM33p16vZT-zM9MA), nkekev, [ATTLAS](https://twitter.com/ATTLAS_), [froid_san](https://froidromhacks.org), [yoti](https://twitter.com/realyoti)
- Marketing: dots-tb
- Product manager: dots-tb
- Author: [浅倉麗子](https://github.com/cuevavirus)

## See more

CBPS ([forum](https://forum.devchroma.nl/index.php), [discord](https://discordapp.com/invite/2ccAkg3))
