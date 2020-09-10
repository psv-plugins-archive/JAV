# 自動オーディオボリューム

Automatically switch between speaker and headphone volumes.

![demo.gif](https://git.shotatoshounenwachigau.moe/vita/jav/plain/demo.gif?h=assets)
![bt_demo.gif](https://git.shotatoshounenwachigau.moe/vita/jav/plain/bt_demo.gif?h=assets)

## Installation

Supported firmware versions:

- Retail 3.60-3.73
- Testkit 3.60, 3.65

Install under `*main` of your taiHEN config.

```
*main
ur0:/tai/jav.suprx
```

## Usage

- When the audio output device is changed, the volume level automatically switches to the last used volume for that device.
- Bluetooth audio devices have volume levels saved per device, for up to 32 devices.
- When a Bluetooth audio device is disconnected, automatic mute follows your setting.
- If your Vita is muted, the volume changes but remains muted. Press the volume buttons to unmute.
- For Europe region Vitas, AVLS does not turn on automatically at boot or after 20 hours of playback.

## Building

Dependencies:

- [DolceSDK](https://forum.devchroma.nl/index.php/topic,129.0.html)
- [taiHEN](https://git.shotatoshounenwachigau.moe/vita/taihen)

## Contributing

Use [git-format-patch](https://www.git-scm.com/docs/git-format-patch) or [git-request-pull](https://www.git-scm.com/docs/git-request-pull) and email me at <asakurareiko@protonmail.ch>.

## Credits

- Plugin idea: [nkekev](https://twitter.com/Nkekev)
- Testing: [dots-tb](https://twitter.com/dots_tb), nkekev, ATTLAS, [froid_san](https://froidromhacks.com), [yoti](https://twitter.com/realyoti)
- Marketing: dots-tb

## See also

- [Discussion](https://forum.devchroma.nl/index.php/topic,46.0.html)
- [Source code](https://git.shotatoshounenwachigau.moe/vita/jav)
