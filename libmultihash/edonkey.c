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

/* As Wikipedia (http://en.wikipedia.org/wiki/Ed2k_link) says:
 * The ed2k hash function is a MD4 root hash of a MD4 hash list, and gives a
 * different result than simply MD4: The file data is divided into full
 * chunks of 9728000 bytes plus a remainder chunk, and a separate 128-bit
 * MD4 checksum is computed for each. The ed2k hash is computed by
 * concatenating the chunks' MD4 checksums in order and hashing the result
 *using MD4.
 */

#include <stdio.h>
#include "edonkey.h"
#include "md4.h"

void ed2khash_starts (ed2khash_context *ctx) {
	md4_starts (&(ctx -> md4cur));
	md4_starts (&(ctx -> md4final));
	ctx -> bytes_processed = 0;
	ctx -> chunks = 0;

	return;
}


void ed2khash_update (ed2khash_context *ctx, unsigned char *input, int ilen) {
	unsigned long x;
	
	while (ilen > 0) {
		if (ctx -> bytes_processed + ilen >= ED2KHASH_CHUNKSIZE)
			x = ED2KHASH_CHUNKSIZE - ctx -> bytes_processed;
		else
			x = ilen;
		
		md4_update (&(ctx -> md4cur), input, x);

		if ((ctx -> bytes_processed += x) % ED2KHASH_CHUNKSIZE == 0) {
			/* End of a chunk, save current MD4 and start a new one */
			md4_finish (&(ctx -> md4cur), ctx -> lastmd4);
			md4_starts (&(ctx -> md4cur));
			ctx -> bytes_processed = 0;
			ctx -> chunks++;

			md4_update (&(ctx -> md4final), ctx -> lastmd4, MD4_DIGESTSIZE);
		}
		ilen -= x;
		input += x;
	}

	return;
}


void ed2khash_finish (ed2khash_context *ctx, unsigned char *output) {
	if (ctx -> chunks > 0) {
		md4_finish (&(ctx -> md4cur), ctx -> lastmd4);
		md4_update (&(ctx -> md4final), ctx -> lastmd4, MD4_DIGESTSIZE);
		md4_finish (&(ctx -> md4final), output);
	} else {
		/* If we have a single chunk, use its MD4 straight away */
		md4_finish (&(ctx -> md4cur), output);
	}

	return;
}
