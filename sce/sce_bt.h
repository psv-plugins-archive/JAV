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

#ifndef SCE_BT_H
#define SCE_BT_H

typedef struct {
// 0x0
	int mac0;
	int mac1;
	int bt_class;
	int bt_profile;
// 0x10
	int unk_10;
	short vid;
	short pid;
	int unk_18;
	int unk_1C;
// 0x20
	char name[0x80];
	char unk_A0[0x60];
// 0x100
} SceBtRegisteredInfo;

extern int sceBtGetRegisteredInfo(int mac0, int mac1, SceBtRegisteredInfo *reg_info, int count);
#define BT_PROFILE_A2DP  0xC
#define BT_PROFILE_AVRCP 0x30
#define BT_PROFILE_HID   0xC0
#define BT_PROFILE_HSP   0x30000

#define BT_CLASS_COMPUTER 1
#define BT_CLASS_PHONE    2
#define BT_CLASS_HEADSET  3
#define BT_CLASS_SPEAKER  4
#define BT_CLASS_MOUSE    5
#define BT_CLASS_KEYBOARD 6
#define BT_CLASS_PRINTER  7
#define BT_CLASS_GAMEPAD  8
#define BT_CLASS_REMOTE   9

extern int sceBtGetConnectingInfo(int mac0, int mac1);
#define BT_CON_DISCONNECTED        1
#define BT_CON_CONNECTING          2
#define BT_CON_CONNECTED_AUDIO_OUT 5
#define BT_CON_CONNECTED           6

extern int sceBtAvrcpReadVolume(int mac0, int mac1);
extern int sceBtAvrcpSendVolume(int mac0, int mac1, int vol);

#define SCE_BT_ERROR_AVCTP_OPEN_NO_L2C     0x802F0601
#define SCE_BT_ERROR_AVCTP_SEND_NO_L2C     0x802F0603
#define SCE_BT_ERROR_AVCTP_NOT_CONNECTED   0x802F0604
#define SCE_BT_ERROR_AVCTP_SEND_BUSY       0x802F0605
#define SCE_BT_ERROR_AVCTP_SEND_NO_PRESS   0x802F0606
#define SCE_BT_ERROR_AVCTP_SEND_NO_RELEASE 0x802F0607
#define SCE_BT_ERROR_AVCTP_READ_NO_VOLUME  0x802F0608
#define SCE_BT_ERROR_AVCTP_SEND_NOT_RUBY   0x802F0609

#endif
