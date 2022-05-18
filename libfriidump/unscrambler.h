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

#ifndef UNSCRAMBLER_H_INCLUDED
#define UNSCRAMBLER_H_INCLUDED

#include "misc.h"
#include <sys/types.h>

typedef struct unscrambler_s unscrambler;

/* We want this module to be independent of the library framework, so that it can be easily recycled. Hence we just define the type of
   the progress function the same format we use elsewhere */
typedef void (*unscrambler_progress_func) (bool start, u_int32_t current_sector, u_int32_t total_sectors, void *progress_data);

//u_int8_t disctype;

FRIIDUMPLIB_EXPORT unscrambler *unscrambler_new (void);
FRIIDUMPLIB_EXPORT void *unscrambler_destroy (unscrambler *u);
FRIIDUMPLIB_EXPORT bool unscrambler_unscramble_16sectors (unscrambler *u, u_int32_t sector_no, u_int8_t *inbuf, u_int8_t *outbuf);
FRIIDUMPLIB_EXPORT bool unscrambler_unscramble_file (unscrambler *u, char *infile, char *outfile, unscrambler_progress_func progress, void *progress_data, u_int32_t *current_sector);
FRIIDUMPLIB_EXPORT void unscrambler_set_bruteforce (unscrambler *u, bool b);
FRIIDUMPLIB_EXPORT void unscrambler_set_disctype (u_int8_t disc_type);

#endif
