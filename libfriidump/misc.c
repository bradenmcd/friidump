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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
//#include <sys/time.h>
#include <time.h>

/*** LOGGING STUFF ***/
/* Uses code from the printf man page */
static int my_vasprintf (char **dst, const char *fmt, va_list ap) {
    char *p;
    int n, size;
    bool done;
    va_list ap2;

    /* Guess we need no more than 100 bytes. */
    size = 100;
    if ((p = (char *) malloc (size)) != NULL) {
        do {
            done = false;
            va_copy (ap2, ap);
            n = vsnprintf (p, size, fmt, ap2);
            va_end (ap2);

            /* If that worked, return the string. */
            if (n > -1 && n < size)
                done = true;
            else {
                /* Else try again with more space. */
                if (n > -1)        /* glibc 2.1 */
                    size = n + 1; /* precisely what is needed */
                else    /* glibc 2.0 */
                    size *= 2;  /* twice the old size */

                if ((p = (char *) realloc (p, size)) == NULL)
                    done = true;
            }
        } while (!done);
    }

    /* Return -1 if memory allocation failed */
    if (!(*dst = p))
        n = -1;

    return (n);
}


/* Appends a print-like string to the log file, with respect to the verbosity setting */
void _logprintf (int level, char tag[], char format[], ...) {
    static bool last_nocr = false;
    char *buffer, t[50];
    va_list ap;
	struct timeval now;
	time_t nowtt;
    struct tm nowtm;

    va_start (ap, format);
    my_vasprintf (&buffer, format, ap);
    va_end (ap);

    /* Start of line, including time and tag */
    if (!last_nocr) {
		gettimeofday (&now, NULL);
		nowtt = (time_t) now.tv_sec;
		if (localtime_r (&nowtt, &nowtm))
	        strftime (t, 50, "%H:%M:%S", &nowtm);
        if (tag)
            fprintf (stderr, "[%s/%s] ", t, tag);
        else
            fprintf (stderr, "[%s] ", t);

        /* Debug level tag */
        if (level & LOG_WARNING)
            fprintf (stderr, "WARNING: ");
        else if (level & LOG_ERROR)
            fprintf (stderr, "ERROR: ");
        #ifdef DEBUG
        else if (level & LOG_DEBUG)
            fprintf (stderr, "DEBUG: ");
        #endif
    }

    /* Actual message */
    fprintf (stderr, "%s%s", buffer, level & LOG_NOCR ? "" : "\n");
    fflush (stderr);
    free (buffer);

    if (level & LOG_NOCR)
        last_nocr = true;
    else
        last_nocr = false;

    return;
}
/******/


/***************** TAKEN FROM TCPDUMP ****************/

#define ASCII_LINELENGTH 300
#define HEXDUMP_BYTES_PER_LINE 16
#define HEXDUMP_SHORTS_PER_LINE (HEXDUMP_BYTES_PER_LINE / 2)
#define HEXDUMP_HEXSTUFF_PER_SHORT 5 /* 4 hex digits and a space */
#define HEXDUMP_HEXSTUFF_PER_LINE (HEXDUMP_HEXSTUFF_PER_SHORT * HEXDUMP_SHORTS_PER_LINE)

void hex_and_ascii_print_with_offset (const char *ident, register const u_int8_t *cp,
                                      register u_int16_t length, register u_int16_t oset) {
    register u_int16_t i;
    register int s1, s2;
    register int nshorts;
    char hexstuff[HEXDUMP_SHORTS_PER_LINE*HEXDUMP_HEXSTUFF_PER_SHORT+1], *hsp;
    char asciistuff[ASCII_LINELENGTH+1], *asp;

    nshorts = length / sizeof(unsigned short);
    i = 0;
    hsp = hexstuff;
    asp = asciistuff;
    while (--nshorts >= 0) {
        s1 = *cp++;
        s2 = *cp++;
        (void)snprintf(hsp, sizeof(hexstuff) - (hsp - hexstuff),
                       " %02x%02x", s1, s2);
        hsp += HEXDUMP_HEXSTUFF_PER_SHORT;
        *(asp++) = (isgraph(s1) ? s1 : '.');
        *(asp++) = (isgraph(s2) ? s2 : '.');
        i++;
        if (i >= HEXDUMP_SHORTS_PER_LINE) {
            *hsp = *asp = '\0';
            (void)printf("%s0x%04x: %-*s  %s",
                         ident, oset, HEXDUMP_HEXSTUFF_PER_LINE,
                         hexstuff, asciistuff);
            i = 0;
            hsp = hexstuff;
            asp = asciistuff;
            oset += HEXDUMP_BYTES_PER_LINE;
        }
    }
    if (length & 1) {
        s1 = *cp++;
        (void)snprintf(hsp, sizeof(hexstuff) - (hsp - hexstuff),
                       " %02x", s1);
        hsp += 3;
        *(asp++) = (isgraph(s1) ? s1 : '.');
        ++i;
    }
    if (i > 0) {
        *hsp = *asp = '\0';
        debug ("%s0x%04x: %-*s  %s",
               ident, oset, HEXDUMP_HEXSTUFF_PER_LINE,
               hexstuff, asciistuff);
    }

    debug ("\n");		// Final spacing
}

char *strtrimr (char *s) {
    int i;

    for (i = strlen (s) - 1; i >= 0 && isspace (s[i]); i--)
        s[i] = '\0';

    return (s);
}


/*** The following was taken from the Python sources */
#if !defined(HAVE_LARGEFILE_SUPPORT)
#error "Large file support must be enabled for this program"
#else

/* A portable fseek() function
   return 0 on success, non-zero on failure (with errno set) */
int my_fseek (FILE *fp, my_off_t offset, int whence) {
    #if defined(HAVE_FSEEKO) && SIZEOF_OFF_T >= 8
    return fseeko(fp, offset, whence);
    #elif defined(_MSC_VER)
    return _fseeki64(fp, (__int64) offset, whence);
    #elif defined(HAVE_FSEEK64)
    return fseek64(fp, offset, whence);
    #elif defined(__BEOS__)
    return _fseek(fp, offset, whence);
    #elif SIZEOF_FPOS_T >= 8
    /* lacking a 64-bit capable fseek(), use a 64-bit capable fsetpos()
       and fgetpos() to implement fseek()*/
    fpos_t pos;

    switch (whence) {
    case SEEK_END:
        #ifdef MS_WINDOWS
        fflush (fp);
        if (_lseeki64 (fileno(fp), 0, 2) == -1)
            return -1;
        #else
        if (fseek (fp, 0, SEEK_END) != 0)
            return -1;
        #endif
        // fall through
    case SEEK_CUR:
        if (fgetpos (fp, &pos) != 0)
            return -1;
        offset += pos;
        break;
        // case SEEK_SET: break;
    }
    return fsetpos(fp, &offset);
    #else
    #error "Large file support, but no way to fseek."
    #endif
}


/* A portable ftell() function
   Return -1 on failure with errno set appropriately, current file
   position on success */
my_off_t my_ftell (FILE* fp) {
    #if defined(HAVE_FTELLO) && SIZEOF_OFF_T >= 8
    return ftello (fp);
    #elif defined(_MSC_VER)
    return _ftelli64 (fp);
    #elif defined(HAVE_FTELL64)
    return ftell64 (fp);
    #elif SIZEOF_FPOS_T >= 8
    fpos_t pos;

    if (fgetpos (fp, &pos) != 0)
        return -1;

    return pos;
    #else
    #error "Large file support, but no way to ftell."
    #endif
}

#endif



/*** STUFF FOR DROPPING PRIVILEGES ***/

/* WARNING: I'm not sure at all that the privileges-dropping system I have implemented is secure, so don't rely too much on it. */

#ifndef WIN32
#include <unistd.h>
#endif

/**
 * Drops privileges to those of the real user (i. e. set euid to ruid).
 */
void drop_euid () {
#ifndef WIN32
	uid_t uid, euid;

	uid = getuid ();
	euid = geteuid ();
	if (uid != 0 && uid != euid) {
#if 1
		seteuid (uid);
#else
		if (seteuid (uid) != 0)
			debug ("seteuid() to uid %d failed", uid);
		else
			debug ("Changed euid from %d to %d", euid, uid);
#endif
	}
#endif

	return;
}


/**
 * Upgrades priviles to those of root (i. e. set euid to 0).
 */
void upgrade_euid () {
#ifndef WIN32
	if (getuid () != 0) {
#if 1
		seteuid (0);
#else
		if (seteuid (0) != 0)
			debug ("seteuid() to root failed");
		else
			debug ("Changed euid to root");
#endif
	}
#endif

       return;
}
