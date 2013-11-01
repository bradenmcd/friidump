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

#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

/*** Include site configuration ***/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/******/

/*** Windows stuff ***/
#include "win32compat.h"

#define _GNU_SOURCE		// For strndup()

#include <stdio.h>
#include <string.h>
#include <sys/types.h>


/*** ASSERTIONS ***/
#define MY_ASSERT(cond) \
        if (!(cond)) { \
                fprintf (stderr, "*** ASSERTION FAILED at " __FILE__ ":%d: " # cond "\n", __LINE__); \
                exit (9); \
        }

/*** LOGGING STUFF ***/
enum {
	LOG_NORMAL		= 1 << 0,
	LOG_WARNING		= 1 << 1,
	LOG_ERROR		= 1 << 2,
#ifdef DEBUG
	LOG_DEBUG		= 1 << 4,
#endif	
	LOG_NOCR 		= 1 << 15	/* Use in OR to avoid a trailing newline */
};

/* Don't use this function explicitly. Use the macros below. */
void _logprintf (int level, char tag[], char format[], ...);

#ifdef DEBUG
	#define logprintf(level, ...) _logprintf (level, (char *) __FUNCTION__, __VA_ARGS__)

	#define debug(...) logprintf(LOG_DEBUG, __VA_ARGS__)
	#define debug_nocr(...) logprintf(LOG_DEBUG | LOG_NOCR, __VA_ARGS__)
#else
	#define logprintf(level, ...) _logprintf (level, NULL, __VA_ARGS__)

	#define debug(...) do {;} while (0);
	#define debug_nocr(...) do {;} while (0);
#endif

#if defined (DEBUG) || defined (VERBOSE)
#define log(...) logprintf (LOG_NORMAL, __VA_ARGS__)
#define warning(...) logprintf (LOG_WARNING, __VA_ARGS__)
#define error(...) logprintf (LOG_ERROR, __VA_ARGS__)
#else
#define log(...)
#define warning(...)
#define error(...)
#endif
/******/

void hex_and_ascii_print_with_offset(const char *ident, register const u_int8_t *cp,
                                                                        register u_int16_t length, register u_int16_t oset);
#define hex_and_ascii_print(ident, cp, length) hex_and_ascii_print_with_offset(ident, cp, length, 0)


#define my_strdup(dest, src) \
	if (!(dest = strdup (src))) { \
		fprintf (stderr, "strdup() failed\n"); \
		exit (101); \
	}

#define my_strndup(dest, src, c) \
	if (!(dest = strndup ((const char *) src, c))) { \
		fprintf (stderr, "strndup() failed\n"); \
		exit (102); \
	}

char *strtrimr (char *s);

#define my_free(p) \
	if (p) { \
		free (p); \
		p = NULL; \
	}


/*** STUFF FOR PORTABLE LARGE-FILES FSEEK ***/
#if defined (HAVE_OFF_T) && SIZEOF_OFF_T >= 8
typedef off_t my_off_t;
#elif defined (HAVE_FPOS_T) && SIZEOF_FPOS_T >= 8
typedef fpos_t my_off_t;
#else
typedef u_int64_t my_off_t;
#endif

int my_fseek (FILE *fp, my_off_t offset, int whence);
my_off_t my_ftell (FILE* fp);
/*******/


/*** STUFF FOR DROPPING PRIVILEGES ***/
FRIIDUMPLIB_EXPORT void drop_euid ();
FRIIDUMPLIB_EXPORT void upgrade_euid ();
/******/


/*** STUFF FOR BOOLEAN DATA TYPE ***/
#ifdef HAVE_STDBOOL_H
/* If using a C99 compiler, use the builtin boolean type */
#include <stdbool.h>
#else
typedef enum {false,true} bool;
#endif
/******/

#endif
