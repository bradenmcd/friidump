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

#include "multihash.h"
#include <stdio.h>

#define READBUF_SIZE 8192


void multihash_init (multihash *mh) {
#ifdef USE_CRC32
	(mh -> crc32_s)[0] = '\0';
	mh -> crc32 = 0xffffffff;
#endif
#ifdef USE_MD4
	(mh -> md4_s)[0] = '\0';
	md4_starts (&(mh -> md4));
#endif
#ifdef USE_MD5
	(mh -> md5_s)[0] = '\0';
	MD5Init (&(mh -> md5));
#endif
#ifdef USE_ED2K
	(mh -> ed2k_s)[0] = '\0';
	ed2khash_starts (&(mh -> ed2k));
#endif
#ifdef USE_SHA1
	(mh -> sha1_s)[0] = '\0';
	SHA1Init (&(mh -> sha1));
#endif

	return;
}


void multihash_update (multihash *mh, unsigned char *data, int bytes) {
#ifdef USE_CRC32
	mh -> crc32 = CrcUpdate (mh -> crc32, data, bytes);
#endif
#ifdef USE_MD4
//	md4_update (&(mh -> md4), data, bytes);
#endif
#ifdef USE_MD5
	MD5Update (&(mh -> md5), data, bytes);
#endif
#ifdef USE_ED2K
//	ed2khash_update (&(mh -> ed2k), data, bytes);
#endif
#ifdef USE_SHA1
	SHA1Update (&(mh -> sha1), data, bytes);		/* WARNING: SHA1Update() destroys data! */
#endif

	return;
}


void multihash_finish (multihash *mh) {
	unsigned char buf[MAX_DIGESTSIZE];
	int bytes;
	
#ifdef USE_CRC32
	mh -> crc32 ^= 0xffffffff;
	snprintf (mh -> crc32_s, LEN_CRC32 + 1, "%08x", mh -> crc32);
#endif
#ifdef USE_MD4
	md4_finish (&(mh -> md4), buf);
	for (bytes = 0; bytes < LEN_MD4 / 2; bytes++)
		sprintf (mh -> md4_s, "%s%02x", mh -> md4_s, buf[bytes]);
	(mh -> md4_s)[LEN_MD4] = '\0';
#endif
#ifdef USE_MD5
	MD5Final (&(mh -> md5));
	for (bytes = 0; bytes < LEN_MD5 / 2; bytes++)
		sprintf (mh -> md5_s, "%s%02x", mh -> md5_s, (mh -> md5).digest[bytes]);
	(mh -> md5_s)[LEN_MD5] = '\0';
#endif
#ifdef USE_ED2K
	ed2khash_finish (&(mh -> ed2k), buf);
	for (bytes = 0; bytes < LEN_ED2K / 2; bytes++)
		sprintf (mh -> ed2k_s, "%s%02x", mh -> ed2k_s, buf[bytes]);
	(mh -> ed2k_s)[LEN_ED2K] = '\0';
#endif	
#ifdef USE_SHA1
	SHA1Final (buf, &(mh -> sha1));
	for (bytes = 0; bytes < LEN_SHA1 / 2; bytes++)
		sprintf (mh -> sha1_s, "%s%02x", mh -> sha1_s, buf[bytes]);
	(mh -> sha1_s)[LEN_SHA1] = '\0';

#endif

	return;
}


int multihash_file (multihash *mh, char *filename) {
	FILE *fp;
	int bytes, out;
	unsigned char data[READBUF_SIZE];
	
	multihash_init (mh);
	if ((fp = fopen (filename, "r"))) {
		while ((bytes = fread (data, 1, READBUF_SIZE, fp)) != 0)
			multihash_update (mh, data, bytes);
		multihash_finish (mh);
		fclose (fp);
		out = 0;
	} else {
		/* Cannot open file */
		out = -1;
	}

	return (out);
}
