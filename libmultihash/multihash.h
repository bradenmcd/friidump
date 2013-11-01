/***************************************************************************
 *   Copyright (C) 2007 by SukkoPera   *
 *   sukkopera@sukkology.net   *
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

#ifndef MULTIHASH_H_INCLUDED
#define MULTIHASH_H_INCLUDED

#ifdef WIN32
#include <windows.h>

/* Definition of the types we use for Windows */
#define u_int8_t BYTE
#define u_int16_t WORD
#define u_int32_t DWORD

/* Some functions have different names */
#define snprintf			_snprintf

/* Stuff to export library symbols */
#ifdef MULTIHASH_BUILD_DLL
#ifdef MULTIHASH_EXPORTS
#define MULTIHASH_EXPORT __declspec(dllexport)  /* Building the lib */
#else
#define MULTIHASH_EXPORT __declspec(dllimport)  /* Building user code */
#endif
#else
#define MULTIHASH_EXPORT
#endif

#else	/* !WIN32 */

#define MULTIHASH_EXPORT

#endif


#ifdef __cplusplus
extern "C" {
#endif

#define USE_CRC32
#define USE_MD4
#define USE_MD5
#define USE_ED2K
#define USE_SHA1

#ifdef USE_CRC32
#include <sys/types.h>
#include "crc32.h"
#define LEN_CRC32 8
#endif

#ifdef USE_MD4
#include "md4.h"
#define LEN_MD4 32
#define MD4_DIGESTSIZE 16
#endif

#ifdef USE_MD5
#include "md5.h"
#define LEN_MD5 32
#endif

#ifdef USE_ED2K
#include "edonkey.h"
#define LEN_ED2K 32
#endif

#ifdef USE_SHA1
#include "sha1.h"
#define LEN_SHA1 40
#endif

/* This must be as long as the longest hash (in bytes) */
#define MAX_DIGESTSIZE 20

typedef struct {
#ifdef USE_CRC32
	u_int32_t crc32;
	char crc32_s[LEN_CRC32 + 1];
#endif
#ifdef USE_MD4
	md4_context md4;
	char md4_s[LEN_MD4 + 1];
#endif
#ifdef USE_MD5
	MD5_CTX md5;
	char md5_s[LEN_MD5 + 1];
#endif
#ifdef USE_ED2K
	ed2khash_context ed2k;
	char ed2k_s[LEN_ED2K + 1];
#endif
#ifdef USE_SHA1
	SHA1_CTX sha1;
	char sha1_s[LEN_SHA1 + 1];
#endif
} multihash;


/* Prototypes */
MULTIHASH_EXPORT void multihash_init (multihash *mh);
MULTIHASH_EXPORT void multihash_update (multihash *mh, unsigned char *data, int bytes);
MULTIHASH_EXPORT void multihash_finish (multihash *mh);
MULTIHASH_EXPORT int multihash_file (multihash *mh, char *filename);

#ifdef __cplusplus
}
#endif

#endif
