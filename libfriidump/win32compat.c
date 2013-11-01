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


#ifdef WIN32

#include "win32compat.h"
#include <stdio.h>
#include <windows.h>
#include <io.h>
#include <sys/timeb.h>


char *strndup (const char *src, int c) {
    char *dest;

    if ((dest = (char *) malloc ((c + 1) * sizeof (char)))) {
        memcpy (dest, src, c);
        dest[c] = '\0';
    }

    return (dest);
}


int ftruncate (int fd, __int64 size) {
	return (_chsize_s (fd, size));
}



/* The following gettimeofday() replacement function was taken from Snort, GPLv2 */
/****************************************************************************
 *
 * Function: gettimeofday(struct timeval *, struct timezone *)
 *
 * Purpose:  Get current time of day.
 *
 * Arguments: tv => Place to store the curent time of day.
 *            tz => Ignored.
 *
 * Returns: 0 => Success.
 *
 ****************************************************************************/
int gettimeofday(struct timeval *tv, struct timezone *tz) {
    struct _timeb tb;

    if (tv == NULL) {
        return -1;
    }
    _ftime(&tb);
    tv->tv_sec = (long) tb.time;
    tv->tv_usec = ((int) tb.millitm) * 1000;
    return 0;
}

#endif
