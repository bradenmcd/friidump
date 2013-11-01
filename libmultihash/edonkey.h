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

#include "md4.h"

#define ED2KHASH_CHUNKSIZE 9728000
#define MD4_DIGESTSIZE 16

typedef struct {
	unsigned long bytes_processed;
	unsigned int chunks;
	md4_context md4cur;
	md4_context md4final;
	unsigned char lastmd4[MD4_DIGESTSIZE];
} ed2khash_context;

void ed2khash_starts (ed2khash_context *ctx);
void ed2khash_update (ed2khash_context *ctx, unsigned char *input, int ilen);
void ed2khash_finish (ed2khash_context *ctx, unsigned char *output);
