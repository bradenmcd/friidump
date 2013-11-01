/***************************************************************************
 *   Copyright (C) 2007 by Arep                                            *
 *   Support is provided through the forums at                             *
 *   http://www.console-tribe.com                                          *
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

#ifndef DUMPER_H_INCLUDED
#define DUMPER_H_INCLUDED

#include "misc.h"
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dumper_s dumper;

typedef void (*progress_func) (bool start, u_int32_t current_sector, u_int32_t total_sectors, void *progress_data);

FRIIDUMPLIB_EXPORT bool dumper_set_raw_output_file (dumper *dmp, char *outfile_raw, bool resume);
FRIIDUMPLIB_EXPORT bool dumper_set_iso_output_file (dumper *dmp, char *outfile_iso, bool resume);
FRIIDUMPLIB_EXPORT bool dumper_prepare (dumper *dmp);
FRIIDUMPLIB_EXPORT int dumper_dump (dumper *dmp, u_int32_t *current_sector);
FRIIDUMPLIB_EXPORT dumper *dumper_new (disc *d);
FRIIDUMPLIB_EXPORT void dumper_set_progress_callback (dumper *dmp, progress_func progress, void *progress_data);
FRIIDUMPLIB_EXPORT void dumper_set_hashing (dumper *dmp, bool h);
FRIIDUMPLIB_EXPORT void dumper_set_flushing (dumper *dmp, bool f);
FRIIDUMPLIB_EXPORT void *dumper_destroy (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_iso_crc32 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_raw_crc32 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_iso_md4 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_raw_md4 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_iso_md5 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_raw_md5 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_iso_ed2k (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_raw_ed2k (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_iso_sha1 (dumper *dmp);
FRIIDUMPLIB_EXPORT char *dumper_get_raw_sha1 (dumper *dmp);

#ifdef __cplusplus
}
#endif

#endif
