/***************************************************************************
 *   Copyright (C) 2007 by Arep                                            *
 *   Support is provided through the forums at                             *
 *   http://wii.console-tribe.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "misc.h"
#include <sys/types.h>

/*************************** BYTE SWAPPING MACROS ***************************/
// WORDS_BIGENDIAN is defined by the AC_C_BIGENDIAN autoconf macro, in case. On Windows please (un)define manually.
#ifdef WORDS_BIGENDIAN		//!< Swapping macros on big endian systems (Where host b. o. = network b. o.)

/* This machine's order to big endian */
#define bo_my2big_16(a) ((u_int16_t)(a))
#define bo_my2big_32(a) ((u_int32_t)(a))

/* This machine's order to little endian */
#define bo_my2little_16(a) ( \
		((((u_int16_t) (a)) & 0x00FF) << 8) + \
		(((u_int16_t) (a)) >> 8) \
	)
#define bo_my2little_32(a) ( \
	        ((((u_int32_t) (a)) & 0x000000FF) << 24) + \
		((((u_int32_t) (a)) & 0x0000FF00) << 8) + \
		((((u_int32_t) (a)) & 0x00FF0000) >> 8) + \
		(((u_int32_t) (a)) >> 24) \
	)
#else				//!< Swapping macros on little endian systems
	/* This machine's order to big endian */
#define bo_my2big_16(a) ( \
		((((u_int16_t) (a)) & 0x00FF) << 8) + \
		(((u_int16_t) (a)) >> 8) \
	)
#define bo_my2big_32(a) ( \
		((((u_int32_t) (a)) & 0x000000FF) << 24) + \
		((((u_int32_t) (a)) & 0x0000FF00) << 8) + \
		((((u_int32_t) (a)) & 0x00FF0000) >> 8) + \
		(((u_int32_t) (a)) >> 24) \
	)

/* This machine's order to little endian */
#define bo_my2little_16(a) ((u_int16_t)(a))
#define bo_my2little_32(a) ((u_int32_t)(a))
#endif

/* These will be handy */
/* Big endian to this machine's order */
#define bo_big2my_16(x) bo_my2big_16(x)
#define bo_big2my_32(x) bo_my2big_32(x)

/* Little endian to this machine's order */
#define bo_little2my_16(x) bo_my2little_16(x)
#define bo_little2my_32(x) bo_my2little_32(x)

/* There are the most useful ones */
#define my_htons(x)	bo_my2big_16(x)
#define my_htonl(x)	bo_my2big_32(x)
#define my_ntohs(x)	my_htons(x)
#define my_ntohl(x)	my_htonl(x)

/************************ END OF BYTE SWAPPING MACROS ***********************/
