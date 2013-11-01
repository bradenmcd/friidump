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

#ifndef DISC_H_INCLUDED
#define DISC_H_INCLUDED

#include "misc.h"
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DISC_TYPE_GAMECUBE,
	DISC_TYPE_WII,
	DISC_TYPE_WII_DL,
	DISC_TYPE_DVD
} disc_type;


typedef enum {
	DISC_REGION_PAL,
	DISC_REGION_NTSC,
	DISC_REGION_JAPAN,
	DISC_REGION_AUSTRALIA,
	DISC_REGION_FRANCE,
	DISC_REGION_GERMANY,
	DISC_REGION_ITALY,
	DISC_REGION_SPAIN,
	DISC_REGION_PAL_X,
	DISC_REGION_PAL_Y,
	DISC_REGION_UNKNOWN
} disc_region;


typedef struct disc_s disc;


/* Functions */
FRIIDUMPLIB_EXPORT disc *disc_new (char *dvd_device, u_int32_t command);
FRIIDUMPLIB_EXPORT bool disc_init (disc *d, u_int32_t forced_type, u_int32_t sectors_no);
FRIIDUMPLIB_EXPORT void *disc_destroy (disc *d);
FRIIDUMPLIB_EXPORT int disc_read_sector (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata);
FRIIDUMPLIB_EXPORT bool disc_set_read_method (disc *d, int method);
FRIIDUMPLIB_EXPORT void disc_set_unscrambling (disc *d, bool unscramble);
FRIIDUMPLIB_EXPORT void disc_set_speed (disc *d, u_int32_t speed);
FRIIDUMPLIB_EXPORT void disc_set_streaming_speed (disc *d, u_int32_t speed);
FRIIDUMPLIB_EXPORT bool disc_stop_unit (disc *d, bool start);
FRIIDUMPLIB_EXPORT void init_range (disc *d, u_int32_t sec_disc, u_int32_t sec_mem);

/* Getters */
FRIIDUMPLIB_EXPORT u_int32_t disc_get_sectors_no (disc *d);
FRIIDUMPLIB_EXPORT u_int32_t disc_get_layerbreak (disc *d);
FRIIDUMPLIB_EXPORT u_int32_t disc_get_command (disc *d);
FRIIDUMPLIB_EXPORT u_int32_t disc_get_method (disc *d);
FRIIDUMPLIB_EXPORT u_int32_t disc_get_def_method (disc *d);
FRIIDUMPLIB_EXPORT u_int32_t disc_get_sec_disc (disc *d);
FRIIDUMPLIB_EXPORT u_int32_t disc_get_sec_mem (disc *d);

FRIIDUMPLIB_EXPORT char *disc_get_type (disc *d, disc_type *dt, char **dt_s);
FRIIDUMPLIB_EXPORT char *disc_get_gameid (disc *d, char **gid_s);
FRIIDUMPLIB_EXPORT char *disc_get_region (disc *d, disc_region *dr, char **dr_s);
FRIIDUMPLIB_EXPORT char *disc_get_maker (disc *d, char **m, char **m_s);
FRIIDUMPLIB_EXPORT char *disc_get_version (disc *d, u_int8_t *v, char **v_s);
FRIIDUMPLIB_EXPORT char *disc_get_title (disc *d, char **t_s);
FRIIDUMPLIB_EXPORT bool disc_get_update (disc *d);

FRIIDUMPLIB_EXPORT char *disc_get_drive_model_string (disc *d);
FRIIDUMPLIB_EXPORT bool disc_get_drive_support_status (disc *d);

#ifdef __cplusplus
}
#endif

#endif
