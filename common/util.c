/*
自動オーディオボリューム
Copyright (C) 2020 浅倉麗子

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

#include "util.h"
#include "log.h"

int decode_bl_t1(int bl, int *imm) {
	// split into two shorts
	short bl_1 = bl & 0xFFFF;
	short bl_2 = (bl >> 16) & 0xFFFF;

	// verify the form
	RNE(bl_1 & 0xF800, 0xF000);
	RNE(bl_2 & 0xD000, 0xD000);

	// decode
	int S = (bl_1 & 0x0400) >> 10;
	int J1 = (bl_2 & 0x2000) >> 13;
	int J2 = (bl_2 & 0x0800) >> 11;
	int I1 = ~(J1 ^ S) & 1;
	int I2 = ~(J2 ^ S) & 1;
	int imm10 = bl_1 & 0x03FF;
	int imm11 = bl_2 & 0x07FF;

	// combine to 25 bits and sign extend
	*imm = (S << 31) | (I1 << 30) | (I2 << 29) | (imm10 << 19) | (imm11 << 8);
	*imm >>= 7;
	return 0;
}
