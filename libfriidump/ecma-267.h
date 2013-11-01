/*
unscrambler 0.4: unscramble not standard IVs scrambled DVDs thru 
bruteforce, intended for Gamecube/WII Optical Disks.

Copyright (C) 2006  Victor Muñoz (xt5@ingenieria-inversa.cl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

typedef unsigned int u32;
typedef int s32;

typedef unsigned short u16;
typedef short s16;

typedef unsigned char u8;
typedef char s8;

/* EDC stuff */

u32 edc_calc(u32 edc, u8 *ptr, u32  len);
 
/* end of EDC stuff */

/* LFSR stuff */

void LFSR_ecma_init(int iv);

void LFSR_init(u16 seed);

int LFSR_tick();

u8 LFSR_byte();

/* end of LFSR stuff */
