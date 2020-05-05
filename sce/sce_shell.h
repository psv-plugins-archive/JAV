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

#ifndef SCE_SHELL_H
#define SCE_SHELL_H

#define VOL_BAR_INIT_MODE_FREE 0x0
#define VOL_BAR_INIT_MODE_INIT 0x1

#define VOL_BAR_FLAG_UNK0  0x1
#define VOL_BAR_FLAG_BT    0x2
#define VOL_BAR_FLAG_AVLS  0x4
#define VOL_BAR_FLAG_MUTED 0x8

typedef struct {
// 0x0
	int *unk_0;
	int *unk_4;
	int *unk_8;
	int *unk_C;
// 0x10
	int *unk_10;
	int *unk_14;
	int *unk_18;
	int volume;
// 0x20
	char muted;
	char avls;
	short pad_22;
	int is_initialised;
// 0x28
} volume_bar_t;

typedef struct {
// 0x0
	int *unk_0;
	int *unk_4;
	float vol_bar_timer; // volume bar is shown until 2.0
	float button_timer; // mute is turned on after 0.8
// 0x10
	char unk_10; // button related
	char unk_11; // button related
	short pad_12;
	int volume;
	int unk_18;
	int vol_ctrl; // from sceAVConfigGetVolCtrlEnable
// 0x20
	volume_bar_t vol_bar;
// 0x48
	char avls;
	char muted;
// 0x4A
} audio_info_t;

#define BUTTON_INFO_UP       0x0001
#define BUTTON_INFO_DOWN     0x0002
#define BUTTON_INFO_LEFT     0x0004
#define BUTTON_INFO_RIGHT    0x0008
#define BUTTON_INFO_CROSS    0x0010
#define BUTTON_INFO_CIRCLE   0x0020
#define BUTTON_INFO_TRIANGLE 0x0040
#define BUTTON_INFO_SQUARE   0x0080
#define BUTTON_INFO_SELECT   0x0100
#define BUTTON_INFO_START    0x0200
#define BUTTON_INFO_LT       0x0400
#define BUTTON_INFO_RT       0x0800
#define BUTTON_INFO_PS       0x1000
#define BUTTON_INFO_VOLUP    0x4000
#define BUTTON_INFO_VOLDOWN  0x8000

typedef struct {
	int *unk_0;
	int *unk_4;
	int buttons;
} button_info2_t;

typedef struct {
	button_info2_t *btn_info_0;
	button_info2_t *btn_info_4;
} button_info_t;

#endif
