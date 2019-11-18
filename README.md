# volume_profile

Automatically switch between your preferred speaker and headphone volumes.

![demo.gif](https://github.com/cuevavirus/volume_profile/raw/assets/demo.gif)

## Usage

- Plug in your headphones, and the volume automatically changes to the volume you last used for your headphones. The opposite happens when you unplug. This works even if they were plugged in or unplugged while the Vita is powered off or suspended.
- If your Vita is muted, the volume changes but remains muted. Press the volume buttons to unmute.
- AVLS functions normally. For European region Vitas, AVLS does not turn back on by itself.

## Installation

Supported firmware versions: 3.60, 3.65-3.73

Download the latest version from [releases](https://github.com/cuevavirus/volume_profile/releases) and install under `*main` of your taiHEN config.

```
*main
volume_profile.suprx
```

This plugin supercedes NoAVLS so noavls.skprx can be removed from under `*KERNEL`.

## Known Issues

- Plugins that modify the input buffer may interfere with detection of headphones ([fix for LOLIcon](https://github.com/cuevavirus/volume_profile/raw/assets/LOLIcon.skprx))


## Credits

- Plugin idea: [nkekev](https://twitter.com/Nkekev)
- Testing: dots-tb, nkekev, [ATTLAS](https://twitter.com/ATTLAS_)
- Marketing: dots-tb
- Product manager: [dots-tb](https://twitter.com/dots_tb)
- Funded by: CBPS ([discord](https://discord.gg/2nDCbxJ))
