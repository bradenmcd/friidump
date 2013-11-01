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


/*** WINDOWS COMPATIBILITY STUFF ***/

#ifdef WIN32
#include <windows.h>
#include <time.h>

/* Stuff to export library symbols */
#ifdef FRIIDUMPLIB_BUILD_DLL
#ifdef FRIIDUMPLIB_EXPORTS
#define FRIIDUMPLIB_EXPORT __declspec(dllexport)  /* Building the lib */
#else
#define FRIIDUMPLIB_EXPORT __declspec(dllimport)  /* Building user code */
#endif
#else
#define FRIIDUMPLIB_EXPORT
#endif

/* Definition of the types we use for Windows */
#define u_int8_t BYTE
#define u_int16_t WORD
#define int32_t INT32
#define u_int32_t DWORD
#define int64_t INT64
#define u_int64_t ULONGLONG
typedef long suseconds_t;

/* Some functions have different names */
#define snprintf _snprintf
#define strdup _strdup
#define strcasecmp lstrcmpi
#define va_copy(ap1, ap2) ((ap1) = (ap2))		// MSVC doesn't have va_copy, how crap...

/* Some functions do not exist (export them for applications, too) */
FRIIDUMPLIB_EXPORT char *strndup (const char *src, int c);
FRIIDUMPLIB_EXPORT int ftruncate (int fd, __int64 size);
FRIIDUMPLIB_EXPORT int gettimeofday(struct timeval *tv, struct timezone *tz);

/* Windows does have a different localtime() */
#define localtime_r(x, y) !localtime_s (y, x)

#else

#define FRIIDUMPLIB_EXPORT

#endif
/******/
