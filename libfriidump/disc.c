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

/*! \file
 * \brief Analyser and dumper for Nintendo GameCube/Wii discs.
 *
 * The functions in this file can be used to retrieve information about a Nintendo GameCube/Wii optical disc. Information is both structural (i.e.: Number of
 * sectors, partitions, etc) and game-related (i.e.: Game Title, version, etc). This is the main object that should be used by applications.
 *
 * Most of the disc structure information used in this file comes from http://www.gc-linux.org/docs/yagcd.html and
 * http://www.wiili.org/index.php/GameCube_Optical_Disc .
 */

#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include "constants.h"
#include "byteorder.h"
#include "disc.h"
#include "dvd_drive.h"
#include "unscrambler.h"

// #define cachedebug(...) debug (__VA_ARGS__);
#define cachedebug(...)


/* Cache always deals with 16-sector blocks. All numbers refer to the 16-sector blocks */
#define DISC_MINIMUM_CACHE_SIZE 5
#define DISC_DEFAULT_CACHE_SIZE 40
#define CACHE_ENTRY_INVALID ((u_int32_t) -1)


#define DISC_GAMECUBE_SECTORS_NO 0x0AE0B0	/* 712880 */
#define DISC_WII_SECTORS_NO_SL 0x230480 	/* 2294912 */
#define DISC_WII_SECTORS_NO_DL 0x3F69C0 	/* 4155840 */


#define MAX_READ_RETRIES 5

#define DEFAULT_READ_METHOD 0
#define DEFAULT_READ_SECTOR disc_read_sector_0


typedef int (*disc_read_sector_func) (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata);

u_int8_t buf[1024*1024*4];
u_int8_t buf_unscrambled[1024*1024*4];

//struct timeval tim;
//double t1, t2;

/*! \brief A structure that represents a Nintendo GameCube/Wii optical disc.
 */
struct disc_s {
	dvd_drive *dvd;				//!< The structure for the DVD-drive the disc is inserted in.
	disc_type type;				//!< The disc type.
	char system_id;				//!< A letter identifying the target system.
	char game_id[2 + 1];			//!< Two letters identifying the game.
	disc_region region;			//!< The disc region.
	char maker[3];				//!< Two letters identifying the maker of the game.
	u_int8_t version;			//!< A number identifying the game version.
	char *version_string;			//!< The same as <code>version</code>, in a more human-understandable format.
	char *title;				//!< The game title.
	bool has_update;			//!< True if the game contains a system update (Only possible for Wii discs).
	u_int32_t sectors_no;			//!< The number of sectors of the disc.
	u_int32_t layerbreak;			//!< For dual-layer DVDs.

	u_int32_t sec_disc;
	u_int32_t sec_mem;
	u_int32_t max_cnt;
	u_int32_t max_blk;

	/* Read function & stuff */
	int command;				//!< Buffer access command ID.
	int read_method;			//!< The read method ID.
//	int def_read_method;		//!< Default read method ID.
	disc_read_sector_func read_sector;	//!< The actual function that will be used to perform read operations, corresponding to <code>read_method</code>.
	bool unscrambling;			//!< If true, raw data read from the disc will be unscrambled to assure it is error-free. Disabling this is only useful for raw performance tests.
	unscrambler *u;				//!< The unscrambler structure that will be used to perform the unscrambling.
	
	/* Read cache */
	u_int32_t cache_size;			//!< The number of blocks that will be cached when read.
	u_int8_t **raw_cache;			//!< Memory area for raw sectors cache.
	u_int8_t **cache;			//!< Memory area for unscrambled sectors cache.
	u_int32_t *cache_map;			//!< Data structure used by the caching system to know which blocks are in memory.
};


static void disc_cache_init (disc *d, u_int32_t size) {
	u_int32_t i;

	if (size < DISC_MINIMUM_CACHE_SIZE) {
		error ("Invalid cache size %u (must be >= %u)", size, DISC_MINIMUM_CACHE_SIZE);
		exit (3);
	} else {
		d -> cache_size = size;
		d -> cache = (u_int8_t **) malloc (sizeof (u_int8_t *) * size);
		d -> raw_cache = (u_int8_t **) malloc (sizeof (u_int8_t *) * size);
		for (i = 0; i < size; i++) {
			d -> cache[i] = (u_int8_t *) malloc (sizeof (u_int8_t) * BLOCK_SIZE);
			d -> raw_cache[i] = (u_int8_t *) malloc (sizeof (u_int8_t) * RAW_BLOCK_SIZE);
		}

		d -> cache_map = (u_int32_t *) malloc (sizeof (u_int32_t) * size);
		for (i = 0; i < size; i++)
			d -> cache_map[i] = CACHE_ENTRY_INVALID;
	}

	return;
}


static void disc_cache_destroy (disc *d) {
	u_int32_t i;
	
	my_free (d -> cache_map);
	
	for (i = 0; i < d -> cache_size; i++) {
			my_free (d -> cache[i]);
			my_free (d -> raw_cache[i]);
	}
	my_free (d -> cache);
	my_free (d -> raw_cache);
	d -> cache_size = 0;

	return;
}


void disc_cache_add_block (disc *d, u_int32_t block, u_int8_t *data, u_int8_t *rawdata) {
	u_int32_t pos;
	u_int32_t cnt;
	
	pos = block % d -> cache_size;
	//uniform unscrambled output
	memcpy (d -> cache[pos], data, BLOCK_SIZE);
	if (d -> type == DISC_TYPE_DVD) {
		for (cnt = 0; cnt < SECTORS_PER_BLOCK; cnt++) {
			memcpy (rawdata+(cnt*RAW_SECTOR_SIZE)+12, data+(cnt*SECTOR_SIZE), SECTOR_SIZE);
		}
	} else {
		for (cnt = 0; cnt < SECTORS_PER_BLOCK; cnt++) {
			memcpy (rawdata+(cnt*RAW_SECTOR_SIZE)+6, data+(cnt*SECTOR_SIZE), SECTOR_SIZE);
		}
	}
	memcpy (d -> raw_cache[pos], rawdata, RAW_BLOCK_SIZE);
	d -> cache_map[pos] = block;

	cachedebug ("Cached block %u (sectors %u-%u) at position %u", block, block * SECTORS_PER_BLOCK, (block + 1) * SECTORS_PER_BLOCK - 1, pos);

	return;
}


static bool disc_cache_lookup_block (disc *d, u_int32_t block, u_int8_t **data, u_int8_t **rawdata) {
	u_int32_t pos;
	bool out;

	pos = block % d -> cache_size;

	if (d -> cache_map[pos] == block) {
		cachedebug ("Cache HIT for block %u", block);
		if (data)
			*data = d -> cache[pos];
		if (rawdata)
			*rawdata = d -> raw_cache[pos];
		out = true;
	} else {
		cachedebug ("Cache MISS for block %u", block);
		if (data)
			*data = NULL;
		if (rawdata)
			*rawdata = NULL;
		out = false;
	}

	return (out);
}


static int disc_read_sector_generic (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata, u_int32_t method) {
	bool out;
	u_int32_t start_block;
	int ret, retry;
	u_int32_t step, cnt, max_cnt, max_blk;
	u_int32_t block_len, block_size, _block_size, last_block_size, block_cnt;
//fprintf (stdout,"disc_read_sector_%d", method);
	start_block = sector_no / SECTORS_PER_BLOCK;

	out = false;
	step = d->sec_mem;
	max_cnt = d->max_cnt;
	max_blk = d->max_blk;

	block_size = step*2064;
	last_block_size = block_size;
	block_len = 1;
	if (block_size > 27 * 2064) {
		block_len = block_size / (27*2064);
		if (block_size % (27*2064) != 0) block_len += 1;
		block_size = 27*2064;
		last_block_size = (step*2064) - (27*2064*(block_len-1));
	}
	_block_size=block_size;

	for (retry = 0; !out && retry < MAX_READ_RETRIES; retry++) {
		/* Assume everything will turn out well */
		out = true;

		//Streaming read
		if (retry < 3) {
			cnt=0;
			while (cnt <= max_cnt){

				_block_size=block_size;
				if (method == 0 || method == 1 || method == 4) {
					if (sector_no+(cnt*step) +992 +16 <= d -> sectors_no) //smaller than last sector
						dvd_read_sector_dummy (d -> dvd, sector_no+(cnt*step) +992, 16, NULL, NULL, 0);
					else if (sector_no+(cnt*step) -992 >= 0)             //larger than first sector
						dvd_read_sector_dummy (d -> dvd, sector_no+(cnt*step) -992, 16, NULL, NULL, 0);
					else dvd_flush_cache_READ12 (d -> dvd, sector_no+(cnt*step), NULL);
				}

				if (method == 0 || method == 2 || method == 5) dvd_flush_cache_READ12 (d -> dvd, sector_no+(cnt*step), NULL);
				if (method == 0 || method == 1 || method == 2 || method == 3) ret = dvd_read_sector_dummy (d -> dvd, sector_no+(cnt*step), d->sec_disc, NULL, &buf_unscrambled[0], 2064*step);
				if (method == 4 || method == 5 || method == 6) ret = dvd_read_streaming (d -> dvd, sector_no+(cnt*step), d->sec_disc, NULL, &buf_unscrambled[0], 2064*step);
				if (ret >= 0) {
					for (block_cnt=0; block_cnt<block_len; block_cnt++) {
						if (dvd_memdump (d -> dvd, block_cnt*27*2064, 1, _block_size, &buf[(cnt*(2064 * step))+(block_cnt*27*2064)]) < 0) {
							error ("Memdump failed");
							//retry = MAX_READ_RETRIES;		/* Well, if this fails going on is useless */ //no it's not!
							out = false;
							break;
						} 
						if (block_cnt==block_len-1) _block_size = last_block_size;
					}
					if (!out) break;
					//do this check only on 1st layer
					else if (((buf[cnt*(2064*step)] & 1) == 0) && ((buf[cnt*(2064*step)+1]<<16)+(buf[cnt*(2064*step)+2]<<8)+(buf[cnt*(2064*step)+3]) != 0x30000 + sector_no+(cnt*step))) {
						out = false;
						break;
					}
					else cnt += 1;
				} else {
					error ("dvd_read_streaming() failed with %d", ret);
					out = false;
					break;
				}

			}
			
			if (cnt < max_cnt) out = false;
			else {
#ifdef DEBUG
				if (d -> unscrambling) {
#endif
					/* Try to unscramble all data to see if EDC fails */
					//for(cnt=0; cnt <= 4; cnt++) {
					for(cnt=max_blk; cnt--;) {
						if (!unscrambler_unscramble_16sectors (d -> u, sector_no+(cnt*16), &buf[cnt*(2064*16)], &buf_unscrambled[cnt*(2048*16)]))
							out = false;
					}
#ifdef DEBUG
				}
#endif
			}
			if (out) {
				/* If data were unscrambled correctly, add them to the cache */
				//for(cnt = 0; cnt <= 4; cnt++) {
				for(cnt=max_blk; cnt--;) {
					disc_cache_add_block (d, start_block+cnt, &buf_unscrambled[cnt*(2048*16)], &buf[cnt*(2064*16)]);
				}
			}
		} //if (retry < 3)

		//Simple read on 4rth try
		else {
			if (sector_no +992 +16 <= d -> sectors_no) //smaller than last sector
				dvd_read_sector_dummy (d -> dvd, sector_no +992, 16, NULL, NULL, 0);
			else if (sector_no -992 >= 0)             //larger than first sector
				dvd_read_sector_dummy (d -> dvd, sector_no -992, 16, NULL, NULL, 0);
			else dvd_flush_cache_READ12 (d -> dvd, sector_no, NULL);

			dvd_flush_cache_READ12 (d -> dvd, sector_no, NULL);
			ret = dvd_read_sector_dummy (d -> dvd, sector_no, SECTORS_PER_BLOCK, NULL, NULL, 0);
			if (ret >= 0) {
				if (dvd_memdump (d -> dvd, 0, 1, RAW_BLOCK_SIZE, buf) < 0) {
					error ("Memdump failed");
					//retry = MAX_READ_RETRIES;		/* Well, if this fails going on is useless */
					out = false;
				} 
				else if ( ((*(buf) & 1) == 0) && ((*(buf+1)<<16)+(*(buf+2)<<8)+(*(buf+3)) != 0x30000+sector_no) ) out = false;
				else {
#ifdef DEBUG
					if (d -> unscrambling) {
#endif
						/* Try to unscramble all data to see if EDC fails */
						if (!unscrambler_unscramble_16sectors (d -> u, sector_no, buf, buf_unscrambled))
							out = false;
#ifdef DEBUG
					}
#endif
				}
				if (out) {
					/* If data were unscrambled correctly, add them to the cache */
					disc_cache_add_block (d, start_block, buf_unscrambled, buf);
				}
			} else {
				error ("dvd_read_sector_dummy() failed with %d", ret);
				out = false;
			}
		} //else
	} //for

	if (!out)
		error ("Too many retries, giving up");

	return (out);
}


///////////////////////////// General /////////////////////////////
static int disc_read_sector_0 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 0);
}



////////////////////////// Non-Streaming //////////////////////////
static int disc_read_sector_1 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 1);

}

static int disc_read_sector_2 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 2);

}


static int disc_read_sector_3 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 3);

}



//////////////////////////// Streaming ////////////////////////////
static int disc_read_sector_4 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 4);
}


static int disc_read_sector_5 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 5);
}


static int disc_read_sector_6 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	return disc_read_sector_generic (d, sector_no, data, rawdata, 6);
}



///////////////////////////// Hitachi /////////////////////////////
static int disc_read_sector_7 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	bool out;
	u_int32_t start_block;
	int j, ret, retry;
	u_int8_t buf[5][16 * 2064];
	u_int8_t buf_unscrambled[5][16 * 2048];
//fprintf (stdout,"disc_read_sector_7");
	start_block = sector_no / SECTORS_PER_BLOCK;

	out = false;
	for (retry = 0; !out && retry < MAX_READ_RETRIES; retry++) {
		/* Assume everything will turn out well */
		out = true;

		if (retry > 0) {
			warning ("Read retry %d for sector %u", retry, sector_no);

			/* Try to reset in-memory data by seeking to a distant sector */
//			if (sector_no > 1000)
//				dvd_read_sector_streaming (d -> dvd, 0, NULL, NULL, 0);
//			else
//				dvd_read_sector_streaming (d -> dvd, 1500, NULL, NULL, 0);
			if (sector_no +992 +16 <= d -> sectors_no) //smaller than last sector
				dvd_read_sector_dummy (d -> dvd, sector_no +992, 16, NULL, NULL, 0);
			else if (sector_no -992 >= 0)             //larger than first sector
				dvd_read_sector_dummy (d -> dvd, sector_no -992, 16, NULL, NULL, 0);
			else dvd_flush_cache_READ12 (d -> dvd, sector_no, NULL);
		}

		if ((ret = dvd_read_sector_streaming (d -> dvd, sector_no, NULL, NULL, 0)) >= 0) {
			for (j = 0; j < 5 && sector_no + j * 16 < d -> sectors_no && out; j++) {
				if (dvd_memdump (d -> dvd, 0 + (j * 16 * 2064), 1, 16 * 2064, buf[j]) < 0) {	/* Dumping in a single block is faster */
					error ("Memdump failed");
					out = false;
					retry = MAX_READ_RETRIES;		/* Well, if this fails going on is useless */
				} else {
#ifdef DEBUG
					if (d -> unscrambling) {
#endif
						/* Try to unscramble all data to see if EDC fails */
						if (!unscrambler_unscramble_16sectors (d -> u, sector_no + (j * 16), buf[j], buf_unscrambled[j]))
							out = false;
#ifdef DEBUG
					}
#endif
				}
			}

			if (out) {
				/* It seems all data was unscrambled correctly, so cache them out */
				for (j = 0; j < 5 && sector_no + j * 16 < d -> sectors_no; j++)
					disc_cache_add_block (d, start_block + j, buf_unscrambled[j], buf[j]);

			}
		} else {
			error ("dvd_read_sector_streaming() failed with %d", ret);
			out = false;
		}
	}

	if (!out)
		error ("Too many retries, giving up");

	return (out);
}


static int disc_read_sector_8 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	bool out;
	u_int32_t ram_offset;
	int j, k, ret, retry;
	u_int8_t *sect, buf[5][RAW_BLOCK_SIZE];
	u_int8_t readbuf[BLOCK_SIZE];
	u_int8_t buf_unscrambled[5][BLOCK_SIZE];
	u_int32_t start_block;
//fprintf (stdout,"disc_read_sector_8");
	start_block = sector_no / SECTORS_PER_BLOCK;
	
	out = false;
	for (retry = 0; !out && retry < MAX_READ_RETRIES; retry++) {
		/* Assume everything will turn out well */
		out = true;

		if (retry > 0) {
			warning ("Read retry %d for sector %u", retry, sector_no);

			/* Try to reset in-memory data by seeking to a distant sector */
//			if (sector_no > 1000)
//				dvd_read_sector_streaming (d -> dvd, 0, NULL, NULL, 0);
//			else
//				dvd_read_sector_streaming (d -> dvd, 1500, NULL, NULL, 0);
			if (sector_no +992 +16 <= d -> sectors_no) //smaller than last sector
				dvd_read_sector_dummy (d -> dvd, sector_no +992, 16, NULL, NULL, 0);
			else if (sector_no -992 >= 0)             //larger than first sector
				dvd_read_sector_dummy (d -> dvd, sector_no -992, 16, NULL, NULL, 0);
			else dvd_flush_cache_READ12 (d -> dvd, sector_no, NULL);
		}

		/* First READ command, this will cache 5 16-sector blocks. Immediately dump relevant data */
		if (sector_no > d -> sectors_no - 1000)
			dvd_read_sector_streaming (d -> dvd, sector_no - 16 * 5 * 2, NULL, NULL, 0);
		else
			dvd_read_sector_streaming (d -> dvd, sector_no + 16 * 5, NULL, NULL, 0);
		if ((ret = dvd_read_sector_streaming (d -> dvd, sector_no, NULL, readbuf, sizeof (readbuf))) >= 0) {
			for (j = 0; j < 5 && sector_no + j * 16 < d -> sectors_no && out; j++) {
				/* Reconstruct raw sectors */
				for (k = 0; k < 16; k++) {
					sect = &buf[j][k * RAW_SECTOR_SIZE];
					ram_offset = (j * RAW_BLOCK_SIZE) + k * RAW_SECTOR_SIZE;
					/* Get first 12 bytes (ID. IED and CPR_MAI fields) and last 4 bytes (EDC field) with memdump */
					if (dvd_memdump (d -> dvd, ram_offset, 1, 12, sect) < 0) {
						error ("Memdump (1) failed");
						out = false;
						retry = MAX_READ_RETRIES;		/* Well, if this fails going on is useless */
					} else if (dvd_memdump (d -> dvd, ram_offset + 2060, 1, 4, sect + 2060) < 0) {	/* Dumping in a single block is faster */
						error ("Memdump (2) failed");
						out = false;
					}
				}
			}

			/* Now the same for remaining 4 16-sector blocks */
			for (j = 0; j < 5 && sector_no + j * 16 < d -> sectors_no && out; j++) {
				if (j == 0 || (ret = dvd_read_sector_streaming (d -> dvd, sector_no + j * 16, NULL, readbuf, sizeof (readbuf))) >= 0) {
					/* Copy "user data" field which has been incorrectly unscrambled by the DVD drive firmware */
					for (k = 0; k < 16; k++) {
						sect = &buf[j][k * RAW_SECTOR_SIZE];
						memcpy (sect + 12, readbuf + k * SECTOR_SIZE, SECTOR_SIZE);
					}
#ifdef DEBUG
					if (d -> unscrambling) {
#endif
						/* Try to unscramble all data to see if EDC fails */
						if (!unscrambler_unscramble_16sectors (d -> u, sector_no + (j * 16), buf[j], buf_unscrambled[j]))
							out = false;
#ifdef DEBUG
					}
#endif
				} else {
					error ("dvd_read_sector_streaming() failed with %d", ret);
					out = false;
				}
			}

			if (out) {
				/* It seems all data were unscrambled correctly, so cache them out */
				for (j = 0; j < 5 && sector_no + j * SECTORS_PER_BLOCK < d -> sectors_no; j++)
					disc_cache_add_block (d, start_block + j, buf_unscrambled[j], buf[j]);
			}
		} else {
			error ("dvd_read_sector_streaming() failed with %d", ret);
			out = false;
		}
	}

	if (!out)
		error ("Too many retries, giving up");

	return (out);
}


static int disc_read_sector_9 (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	bool out;
	u_int32_t ram_offset;
	int j, k, ret, retry;
	u_int8_t *sect, buf[5][RAW_BLOCK_SIZE];
	u_int8_t readbuf[BLOCK_SIZE], tmp[16];
	u_int8_t buf_unscrambled[5][BLOCK_SIZE];
	u_int32_t start_block;
//fprintf (stdout,"disc_read_sector_9");
	start_block = sector_no / SECTORS_PER_BLOCK;

	out = false;
	for (retry = 0; !out && retry < MAX_READ_RETRIES; retry++) {
		/* Assume everything will turn out well */
		out = true;

		if (retry > 0) {
			warning ("Read retry %d for sector %u", retry, sector_no);

			/* Try to reset in-memory data by seeking to a distant sector */
//			if (sector_no > 1000)
//				dvd_read_sector_streaming (d -> dvd, 0, NULL, NULL, 0);
//			else
//				dvd_read_sector_streaming (d -> dvd, 1500, NULL, NULL, 0);
			if (sector_no +992 +16 <= d -> sectors_no) //smaller than last sector
				dvd_read_sector_dummy (d -> dvd, sector_no +992, 16, NULL, NULL, 0);
			else if (sector_no -992 >= 0)             //larger than first sector
				dvd_read_sector_dummy (d -> dvd, sector_no -992, 16, NULL, NULL, 0);
			else dvd_flush_cache_READ12 (d -> dvd, sector_no, NULL);
		}

		/* First READ command, this will cache 5 16-sector blocks. Immediately dump relevant data */
		if (sector_no > d -> sectors_no - 1000)
			dvd_read_sector_streaming (d -> dvd, sector_no - 16 * 5 * 2, NULL, NULL, 0);
		else
			dvd_read_sector_streaming (d -> dvd, sector_no + 16 * 5, NULL, NULL, 0);
		if ((ret = dvd_read_sector_streaming (d -> dvd, sector_no, NULL, readbuf, BLOCK_SIZE)) >= 0) {
			for (j = 0; j < 5 && sector_no + j * 16 < d -> sectors_no && out; j++) {
				/* Reconstruct raw sectors */
				for (k = 0; k < 16; k++) {
					sect = &buf[j][k * RAW_SECTOR_SIZE];
					ram_offset = (j * RAW_BLOCK_SIZE) + k * RAW_SECTOR_SIZE;
					/* Get first 12 bytes (ID. IED and CPR_MAI fields) and last 4 bytes (EDC field) with memdump */
					if (j == 0 && k == 0) {
						if (dvd_memdump (d -> dvd, ram_offset, 1, 12, sect) < 0) {
							error ("Memdump (1) failed");
							out = false;
							retry = MAX_READ_RETRIES;		/* Well, if this fails going on is useless */
						}
					} else {
						memcpy (sect, tmp + 4, 12);
					}
					
					if (out && dvd_memdump (d -> dvd, ram_offset + 2060, 1, 16, tmp) < 0) {	/* Dumping in a single block is faster */
						error ("Memdump (2) failed");
						out = false;
					} else {
						memcpy (sect + 2060, tmp, 4);
					}
				}
			}

			/* Now the same for remaining 4 16-sector blocks */
			for (j = 0; j < 5 && sector_no + j * 16 < d -> sectors_no && out; j++) {
				if (j == 0 || (ret = dvd_read_sector_streaming (d -> dvd, sector_no + j * 16, NULL, readbuf, BLOCK_SIZE)) >= 0) {
					/* Copy "user data" field which has been incorrectly unscrambled by the DVD drive firmware */
					for (k = 0; k < 16; k++) {
						sect = &buf[j][k * RAW_SECTOR_SIZE];
						memcpy (sect + 12, readbuf + k * SECTOR_SIZE, SECTOR_SIZE);
					}
#ifdef DEBUG
					if (d -> unscrambling) {
#endif
						/* Try to unscramble all data to see if EDC fails */
						if (!unscrambler_unscramble_16sectors (d -> u, sector_no + (j * 16), buf[j], buf_unscrambled[j]))
							out = false;
#ifdef DEBUG
					}
#endif
				} else {
					error ("dvd_read_sector_streaming() failed with %d", ret);
					out = false;
				}
			}

			if (out) {
				/* It seems all data were unscrambled correctly, so cache them out */
				for (j = 0; j < 5 && sector_no + j * SECTORS_PER_BLOCK < d -> sectors_no; j++)
					disc_cache_add_block (d, start_block + j, buf_unscrambled[j], buf[j]);
			}
		} else {
			error ("dvd_read_sector_streaming() failed with %d", ret);
			out = false;
		}
	}

	if (!out)
		error ("Too many retries, giving up");

	return (out);
}


/* We could also use the 'System ID' (first byte of the image) to tell the discs apart */
static disc_type disc_detect_type (disc *d, u_int32_t forced_type, u_int32_t sectors_no) {
	req_sense sense;

	if (forced_type==0) {
		d -> type = DISC_TYPE_GAMECUBE;
		d -> sectors_no = DISC_GAMECUBE_SECTORS_NO;
	} else if (forced_type==1) {
		d -> type = DISC_TYPE_WII;
		d -> sectors_no = DISC_WII_SECTORS_NO_SL;
	} else if (forced_type==2) {
		d -> type = DISC_TYPE_WII_DL;
		d -> sectors_no = DISC_WII_SECTORS_NO_DL;
		//dvd_get_layerbreak(d->dvd, &(d -> layerbreak), NULL);
	} else if (forced_type==3) {
		d -> type = DISC_TYPE_DVD;
		if (sectors_no == -1) dvd_get_size(d->dvd, &(d -> sectors_no), NULL);
		dvd_get_layerbreak(d->dvd, &(d -> layerbreak), NULL);
	} else {

	/* Try to read a sector beyond the end of GameCube discs */
	if (!dvd_read_sector_dummy (d -> dvd, DISC_GAMECUBE_SECTORS_NO + 100, SECTORS_PER_BLOCK, &sense, NULL, 0) && sense.sense_key == 0x05 && sense.asc == 0x21) {
		d -> type = DISC_TYPE_GAMECUBE;
		d -> sectors_no = DISC_GAMECUBE_SECTORS_NO;
	} else {
		if (!dvd_read_sector_dummy (d -> dvd, DISC_WII_SECTORS_NO_SL + 100, SECTORS_PER_BLOCK, &sense, NULL, 0) && sense.sense_key == 0x05 && sense.asc == 0x21) {
			d -> type = DISC_TYPE_WII;
			d -> sectors_no = DISC_WII_SECTORS_NO_SL;
		} else {
			d -> type = DISC_TYPE_WII_DL;
			d -> sectors_no = DISC_WII_SECTORS_NO_DL;
			//dvd_get_layerbreak(d->dvd, &(d -> layerbreak), NULL);
		}
	}

	}
	if (sectors_no != -1) d -> sectors_no = sectors_no;

	return (d -> type);
}


/**
 * Reads a sector from the disc (or from the cache), using the preset read method.
 * @param d The disc structure.
 * @param sector_no The requested sector number.
 * @param data A buffer to hold the unscrambled sector data (or NULL).
 * @param rawdata A buffer to hold the raw sector data (or NULL).
 * @return 
 */
int disc_read_sector (disc *d, u_int32_t sector_no, u_int8_t **data, u_int8_t **rawdata) {
	u_int32_t block;
	u_int8_t *cdata, *crawdata;
	int out;

	/* Unscrambled data cannot be requested if unscrambling was disabled */
	MY_ASSERT (!(data && !d -> unscrambling));
	
	block = sector_no / SECTORS_PER_BLOCK;
	
	/* See if sector is in cache */
	if (!(out = disc_cache_lookup_block (d, block, &cdata, &crawdata))) {
		/* Requested block is not in cache, try to read it from media */
		out = d -> read_sector (d, sector_no, data, rawdata);
		
		/* Now requested sector is in cache, for sure ;) */
		if (out)
			MY_ASSERT (disc_cache_lookup_block (d, block, &cdata, &crawdata));
	}

	if (out) {
		if (data)
			*data = cdata + (sector_no % SECTORS_PER_BLOCK) * SECTOR_SIZE;
		if (rawdata)
			*rawdata = crawdata + (sector_no % SECTORS_PER_BLOCK) * RAW_SECTOR_SIZE;
	} else {
		if (data)
			*data = NULL;
		if (rawdata)
			*rawdata = NULL;
	}
		
	return (out);
}


static bool disc_analyze (disc *d) {
	u_int8_t *buf;
	char tmp[0x03E0 + 1];
	bool unscramble_old, out;

	/* Force unscrambling for this read */
	unscramble_old = d -> unscrambling;
	disc_set_unscrambling (d, true);
	
	if (disc_read_sector (d, 0, &buf, NULL)) {
		/* System ID */
		d -> system_id = buf[0];
// 		if (d -> system_id == 'G') {
// 			d -> type = DISC_TYPE_GAMECUBE;
// 			d -> sectors_no = DISC_GAMECUBE_SECTORS_NO;
// 		} else if (d -> system_id == 'R') {
// 			d -> type = DISC_TYPE_WII;
// 			d -> sectors_no = DISC_WII_SECTORS_NO;
// 		} else {
// 			error ("Unknown system ID: '%c'", d -> system_id);
// 			MY_ASSERT (false);
// 		}

		/* Game ID */
		strncpy (d -> game_id, (char *) buf + 1, 2);
		d -> game_id[2] = '\0';

		/* Region */
		switch (buf[3]) {
			case 'P':
				d -> region = DISC_REGION_PAL;
				break;
			case 'E':
				d -> region = DISC_REGION_NTSC;
				break;
			case 'J':
				d -> region = DISC_REGION_JAPAN;
				break;
			case 'U':
				d -> region = DISC_REGION_AUSTRALIA;
				break;
			case 'F':
				d -> region = DISC_REGION_FRANCE;
				break;
			case 'D':
				d -> region = DISC_REGION_GERMANY;
				break;
			case 'I':
				d -> region = DISC_REGION_ITALY;
				break;
			case 'S':
				d -> region = DISC_REGION_SPAIN;
				break;
			case 'X':
				d -> region = DISC_REGION_PAL_X;
				break;
			case 'Y':
				d -> region = DISC_REGION_PAL_Y;
				break;
			default:
				d -> region = DISC_REGION_UNKNOWN;
				break;
		}

		/* Maker code */
		strncpy (d -> maker, (char *) buf + 4, 2);
		d -> maker[2] = '\0';

		/* Version */
		d -> version = buf[7];
		snprintf (tmp, sizeof (tmp), "1.%02u", d -> version);
		my_strdup (d -> version_string, tmp);

		/* Game title */
		memcpy (tmp, buf + 0x0020, sizeof (tmp) - 1);
		tmp[sizeof (tmp) - 1] = '\0';
		strtrimr (tmp);
		my_strdup (d -> title, tmp);

		out = true;
	} else {
		error ("Cannot analyze disc");
		out = false;
	}

	disc_set_unscrambling (d, unscramble_old);

	return (out);
}


static char disc_type_strings[4][15] = {
	"GameCube",
	"Wii",
	"Wii_DL",
	"DVD"
};

/**
 * Retrieves the disc type.
 * @param d The disc structure.
 * @param dt This will be set to the disc type.
 * @param dt_s This will point to a string describing the disc type.
 * @return A string describing the disc type.
 */
char *disc_get_type (disc *d, disc_type *dt, char **dt_s) {
	if (dt)
		*dt = d -> type;

	if (dt_s) {
		if (d -> type < DISC_TYPE_DVD)
			*dt_s = disc_type_strings[d -> type];
		else
			*dt_s = disc_type_strings[DISC_TYPE_DVD];
	}

	return (*dt_s);
}


/**
 * Retrieves the disc game ID.
 * @param d The disc structure.
 * @param gid_s This will point to a string containing the game ID.
 * @return A string containing the game ID.
 */
char *disc_get_gameid (disc *d, char **gid_s) {
	if (gid_s)
		*gid_s = d -> game_id;

	return (*gid_s);
}


static char disc_region_strings[11][15] = {
	"Europe/PAL",
	"USA/NTSC",
	"Japan/NTSC",
	"Australia/PAL",
	"France/PAL",
	"Germany/PAL",
	"Italy/PAL",
	"Spain/PAL",
	"Europe(X)/PAL",
	"Europe(Y)/PAL",
	"Unknown"
};

/**
 * Retrieves the disc region.
 * @param d The disc structure.
 * @param dr This will be set to the disc region.
 * @param dr_s This will point to a string describing the disc region.
 * @return A string describing the disc region.
 */
char *disc_get_region (disc *d, disc_region *dr, char **dr_s) {
	if (dr)
		*dr = d -> region;
	
	if (dr_s) {
		if (d -> region < DISC_REGION_UNKNOWN)
			*dr_s = disc_region_strings[d -> region];
		else
			*dr_s = disc_region_strings[DISC_REGION_UNKNOWN];
	}

	return (*dr_s);
}


/* The following list has been derived from http://wiitdb.com/Company/HomePage */
static struct {
	char *code;
	char *name;
} makers[] = {
	{"0A", "Jaleco"},
	{"0B", "Coconuts Japan"},
	{"0C", "Coconuts Japan / G.X.Media"},
	{"0D", "Micronet"},
	{"0E", "Technos"},
	{"0F", "Mebio Software"},
	{"0G", "Shouei System"},
	{"0H", "Starfish"},
	{"0J", "Mitsui Fudosan / Dentsu"},
	{"0L", "Warashi Inc."},
	{"0N", "Nowpro"},
	{"0P", "Game Village"},
	{"0Q", "IE Institute"},
	{"01", "Nintendo"},
	{"02", "Rocket Games / Ajinomoto"},
	{"03", "Imagineer-Zoom"},
	{"04", "Gray Matter"},
	{"05", "Zamuse"},
	{"06", "Falcom"},
	{"07", "Enix"},
	{"08", "Capcom"},
	{"09", "Hot B Co."},
	{"1A", "Yanoman"},
	{"1C", "Tecmo Products"},
	{"1D", "Japan Glary Business"},
	{"1E", "Forum / OpenSystem"},
	{"1F", "Virgin Games (Japan)"},
	{"1G", "SMDE"},
	{"1J", "Daikokudenki"},
	{"1P", "Creatures Inc."},
	{"1Q", "TDK Deep Impresion"},
	{"2A", "Culture Brain"},
	{"2C", "Palsoft"},
	{"2D", "Visit Co.,Ltd."},
	{"2E", "Intec"},
	{"2F", "System Sacom"},
	{"2G", "Poppo"},
	{"2H", "Ubisoft Japan"},
	{"2J", "Media Works"},
	{"2K", "NEC InterChannel"},
	{"2L", "Tam"},
	{"2M", "Jordan"},
	{"2N", "Smilesoft / Rocket"},
	{"2Q", "Mediakite"},
	{"3B", "Arcade Zone Ltd"},
	{"3C", "Entertainment International / Empire Software"},
	{"3D", "Loriciel"},
	{"3E", "Gremlin Graphics"},
	{"3F", "K.Amusement Leasing Co."},
	{"4B", "Raya Systems"},
	{"4C", "Renovation Products"},
	{"4D", "Malibu Games"},
	{"4F", "Eidos"},
	{"4G", "Playmates Interactive"},
	{"4J", "Fox Interactive"},
	{"4K", "Time Warner Interactive"},
	{"4Q", "Disney Interactive"},
	{"4S", "Black Pearl"},
	{"4U", "Advanced Productions"},
	{"4X", "GT Interactive"},
	{"4Y", "RARE"},
	{"4Z", "Crave Entertainment"},
	{"5A", "Mindscape / Red Orb Entertainment"},
	{"5B", "Romstar"},
	{"5C", "Taxan"},
	{"5D", "Midway / Tradewest"},
	{"5F", "American Softworks"},
	{"5G", "Majesco Sales Inc"},
	{"5H", "3DO"},
	{"5K", "Hasbro"},
	{"5L", "NewKidCo"},
	{"5M", "Telegames"},
	{"5N", "Metro3D"},
	{"5P", "Vatical Entertainment"},
	{"5Q", "LEGO Media"},
	{"5S", "Xicat Interactive"},
	{"5T", "Cryo Interactive"},
	{"5W", "Red Storm Entertainment"},
	{"5X", "Microids"},
	{"5Z", "Data Design / Conspiracy / Swing"},
	{"6B", "Laser Beam"},
	{"6E", "Elite Systems"},
	{"6F", "Electro Brain"},
	{"6G", "The Learning Company"},
	{"6H", "BBC"},
	{"6J", "Software 2000"},
	{"6K", "UFO Interactive Games"},
	{"6L", "BAM! Entertainment"},
	{"6M", "Studio 3"},
	{"6Q", "Classified Games"},
	{"6S", "TDK Mediactive"},
	{"6U", "DreamCatcher"},
	{"6V", "JoWood Produtions"},
	{"6W", "Sega"},
	{"6X", "Wannado Edition"},
	{"6Y", "LSP (Light & Shadow Prod.)"},
	{"6Z", "ITE Media"},
	{"7A", "Triffix Entertainment"},
	{"7C", "Microprose Software"},
	{"7D", "Sierra / Universal Interactive"},
	{"7F", "Kemco"},
	{"7G", "Rage Software"},
	{"7H", "Encore"},
	{"7J", "Zoo"},
	{"7K", "Kiddinx"},
	{"7L", "Simon & Schuster Interactive"},
	{"7M", "Asmik Ace Entertainment Inc."},
	{"7N", "Empire Interactive"},
	{"7Q", "Jester Interactive"},
	{"7S", "Rockstar Games"},
	{"7T", "Scholastic"},
	{"7U", "Ignition Entertainment"},
	{"7V", "Summitsoft"},
	{"7W", "Stadlbauer"},
	{"8B", "BulletProof Software (BPS)"},
	{"8C", "Vic Tokai Inc."},
	{"8E", "Character Soft"},
	{"8F", "I'Max"},
	{"8G", "Saurus"},
	{"8J", "General Entertainment"},
	{"8N", "Success"},
	{"8P", "Sega Japan"},
	{"9A", "Nichibutsu / Nihon Bussan"},
	{"9B", "Tecmo"},
	{"9C", "Imagineer"},
	{"9F", "Nova"},
	{"9G", "Take2 / Den'Z / Global Star"},
	{"9H", "Bottom Up"},
	{"9J", "TGL (Technical Group Laboratory)"},
	{"9L", "Hasbro Japan"},
	{"9N", "Marvelous Entertainment"},
	{"9P", "Keynet Inc."},
	{"9Q", "Hands-On Entertainment"},
	{"12", "Infocom"},
	{"13", "Electronic Arts Japan"},
	{"15", "Cobra Team"},
	{"16", "Human / Field"},
	{"17", "KOEI"},
	{"18", "Hudson Soft"},
	{"19", "S.C.P."},
	{"20", "Destination Software / Zoo Games / KSS"},
	{"21", "Sunsoft / Tokai Engineering"},
	{"22", "POW (Planning Office Wada) / VR1 Japan"},
	{"23", "Micro World"},
	{"25", "San-X"},
	{"26", "Enix"},
	{"27", "Loriciel / Electro Brain"},
	{"28", "Kemco Japan"},
	{"29", "Seta"},
	{"30", "Viacom"},
	{"31", "Carrozzeria"},
	{"32", "Dynamic"},
	{"34", "Magifact"},
	{"35", "Hect"},
	{"36", "Codemasters"},
	{"37", "Taito / GAGA Communications"},
	{"38", "Laguna"},
	{"39", "Telstar / Event / Taito"},
	{"40", "Seika Corp."},
	{"41", "Ubi Soft Entertainment"},
	{"42", "Sunsoft US"},
	{"44", "Life Fitness"},
	{"46", "System 3"},
	{"47", "Spectrum Holobyte"},
	{"49", "IREM"},
	{"50", "Absolute Entertainment"},
	{"51", "Acclaim"},
	{"52", "Activision"},
	{"53", "American Sammy"},
	{"54", "Take 2 Interactive / GameTek"},
	{"55", "Hi Tech"},
	{"56", "LJN LTD."},
	{"58", "Mattel"},
	{"60", "Titus"},
	{"61", "Virgin Interactive"},
	{"62", "Maxis"},
	{"64", "LucasArts Entertainment"},
	{"67", "Ocean"},
	{"68", "Bethesda Softworks"},
	{"69", "Electronic Arts"},
	{"70", "Atari (Infogrames)"},
	{"71", "Interplay"},
	{"72", "JVC (US)"},
	{"73", "Parker Brothers"},
	{"75", "Sales Curve (Storm / SCI)"},
	{"78", "THQ"},
	{"79", "Accolade"},
	{"80", "Misawa"},
	{"81", "Teichiku"},
	{"82", "Namco Ltd."},
	{"83", "LOZC"},
	{"84", "KOEI"},
	{"86", "Tokuma Shoten Intermedia"},
	{"87", "Tsukuda Original"},
	{"88", "DATAM-Polystar"},
	{"90", "Takara Amusement"},
	{"91", "Chun Soft"},
	{"92", "Video System / Mc O' River"},
	{"93", "BEC"},
	{"95", "Varie"},
	{"96", "Yonezawa / S'pal"},
	{"97", "Kaneko"},
	{"99", "Marvelous Entertainment"},
	{"A0", "Telenet"},
	{"A1", "Hori"},
	{"A4", "Konami"},
	{"A5", "K.Amusement Leasing Co."},
	{"A6", "Kawada"},
	{"A7", "Takara"},
	{"A9", "Technos Japan Corp."},
	{"AA", "JVC / Victor"},
	{"AC", "Toei Animation"},
	{"AD", "Toho"},
	{"AF", "Namco"},
	{"AG", "Media Rings Corporation"},
	{"AH", "J-Wing"},
	{"AJ", "Pioneer LDC"},
	{"AK", "KID"},
	{"AL", "Mediafactory"},
	{"AP", "Infogrames / Hudson"},
	{"AQ", "Kiratto. Ludic Inc"},
	{"B0", "Acclaim Japan"},
	{"B1", "ASCII"},
	{"B2", "Bandai"},
	{"B4", "Enix"},
	{"B6", "HAL Laboratory"},
	{"B7", "SNK"},
	{"B9", "Pony Canyon"},
	{"BA", "Culture Brain"},
	{"BB", "Sunsoft"},
	{"BC", "Toshiba EMI"},
	{"BD", "Sony Imagesoft"},
	{"BF", "Sammy"},
	{"BG", "Magical"},
	{"BH", "Visco"},
	{"BJ", "Compile"},
	{"BL", "MTO Inc."},
	{"BN", "Sunrise Interactive"},
	{"BP", "Global A Entertainment"},
	{"BQ", "Fuuki"},
	{"C0", "Taito"},
	{"C2", "Kemco"},
	{"C3", "Square"},
	{"C4", "Tokuma Shoten"},
	{"C5", "Data East"},
	{"C6", "Tonkin House / Tokyo Shoseki"},
	{"C8", "Koei"},
	{"CA", "Konami / Ultra / Palcom"},
	{"CB", "NTVIC / VAP"},
	{"CC", "Use Co.,Ltd."},
	{"CD", "Meldac"},
	{"CE", "Pony Canyon / FCI"},
	{"CF", "Angel / Sotsu Agency / Sunrise"},
	{"CG", "Yumedia / Aroma Co., Ltd"},
	{"CJ", "Boss"},
	{"CK", "Axela / Crea-Tech"},
	{"CL", "Sekaibunka-Sha / Sumire Kobo / Marigul Management Inc."},
	{"CM", "Konami Computer Entertainment Osaka"},
	{"CN", "NEC Interchannel"},
	{"CP", "Enterbrain"},
	{"CQ", "From Software"},
	{"D0", "Taito / Disco"},
	{"D1", "Sofel"},
	{"D2", "Quest / Bothtec"},
	{"D3", "Sigma"},
	{"D4", "Ask Kodansha"},
	{"D6", "Naxat"},
	{"D7", "Copya System"},
	{"D8", "Capcom Co., Ltd."},
	{"D9", "Banpresto"},
	{"DA", "Tomy"},
	{"DB", "LJN Japan"},
	{"DD", "NCS"},
	{"DE", "Human Entertainment"},
	{"DF", "Altron"},
	{"DG", "Jaleco"},
	{"DH", "Gaps Inc."},
	{"DN", "Elf"},
	{"DQ", "Compile Heart"},
	{"E0", "Jaleco"},
	{"E2", "Yutaka"},
	{"E3", "Varie"},
	{"E4", "T&ESoft"},
	{"E5", "Epoch"},
	{"E7", "Athena"},
	{"E8", "Asmik"},
	{"E9", "Natsume"},
	{"EA", "King Records"},
	{"EB", "Atlus"},
	{"EC", "Epic / Sony Records"},
	{"EE", "IGS (Information Global Service)"},
	{"EG", "Chatnoir"},
	{"EH", "Right Stuff"},
	{"EL", "Spike"},
	{"EM", "Konami Computer Entertainment Tokyo"},
	{"EN", "Alphadream Corporation"},
	{"EP", "Sting"},
	{"ES", "Star-Fish"},
	{"F0", "A Wave"},
	{"F1", "Motown Software"},
	{"F2", "Left Field Entertainment"},
	{"F3", "Extreme Ent. Grp."},
	{"F4", "TecMagik"},
	{"F9", "Cybersoft"},
	{"FB", "Psygnosis"},
	{"FE", "Davidson / Western Tech."},
	{"FK", "The Game Factory"},
	{"FL", "Hip Games"},
	{"FM", "Aspyr"},
	{"FP", "Mastiff"},
	{"FQ", "iQue"},
	{"FR", "Digital Tainment Pool"},
	{"FS", "XS Games / Jack Of All Games"},
	{"FT", "Daiwon"},
	{"G0", "Alpha Unit"},
	{"G1", "PCCW Japan"},
	{"G2", "Yuke's Media Creations"},
	{"G4", "KiKi Co Ltd"},
	{"G5", "Open Sesame Inc"},
	{"G6", "Sims"},
	{"G7", "Broccoli"},
	{"G8", "Avex"},
	{"G9", "D3 Publisher"},
	{"GB", "Konami Computer Entertainment Japan"},
	{"GD", "Square-Enix"},
	{"GE", "KSG"},
	{"GF", "Micott & Basara Inc."},
	{"GH", "Orbital Media"},
	{"GJ", "Detn8 Games"},
	{"GL", "Gameloft / Ubi Soft"},
	{"GM", "Gamecock Media Group"},
	{"GN", "Oxygen Games"},
	{"GT", "505 Games"},
	{"GY", "The Game Factory"},
	{"H1", "Treasure"},
	{"H2", "Aruze"},
	{"H3", "Ertain"},
	{"H4", "SNK Playmore"},
	{"HJ", "Genius Products"},
	{"HY", "Reef Entertainment"},
	{"HZ", "Nordcurrent"},
	{"IH", "Yojigen"},
	{"J9", "AQ Interactive"},
	{"JF", "Arc System Works"},
	{"JW", "Atari"},
	{"K6", "Nihon System"},
	{"KB", "NIS America"},
	{"KM", "Deep Silver"},
	{"LH", "Trend Verlag / East Entertainment"},
	{"LT", "Legacy Interactive"},
	{"MJ", "Mumbo Jumbo"},
	{"MR", "Mindscape"},
	{"MS", "Milestone / UFO Interactive"},
	{"MT", "Blast !"},
	{"N9", "Terabox"},
	{"NK", "Neko Entertainment / Diffusion / Naps team"},
	{"NP", "Nobilis"},
	{"NR", "Data Design / Destineer Studios"},
	{"PL", "Playlogic"},
	{"RM", "Rondomedia"},
	{"RS", "Warner Bros. Interactive Entertainment Inc."},
	{"RT", "RTL Games"},
	{"RW", "RealNetworks"},
	{"S5", "Southpeak Interactive"},
	{"SP", "Blade Interactive Studios"},
	{"SV", "SevenGames"},
	{"TK", "Tasuke / Works"},
	{"UG", "Metro 3D / Data Design"},
	{"VN", "Valcon Games"},
	{"VP", "Virgin Play"},
	{"WR", "Warner Bros. Interactive Entertainment Inc."},
	{"XJ", "Xseed Games"},
	{"XS", "Aksys Games"},
	{NULL, NULL}
};

/**
 * Retrieves the disk maker.
 * @param d The disc structure.
 * @param m This will point to a string containing the disc maker ID.
 * @param m_s This will point to a string describing the disc maker.
 * @return A string describing the disc maker.
 */
char *disc_get_maker (disc *d, char **m, char **m_s) {
	u_int32_t i;
	
	if (m)
		*m = d -> maker;

	if (m_s) {
		for (i = 0; makers[i].code; i++) {
			if (strcasecmp (d -> maker, makers[i].code) == 0) {
				*m_s = makers[i].name;
				break;
			}
		}
		if (!makers[i].code) {
			*m_s = "Unknown";
		}
	}

	return (*m_s);
}


/**
 * Retrieves the disc version.
 * @param d The disc structure.
 * @param v This will contain the version ID.
 * @param v_s This will point to a string describing the disc version.
 * @return A string describing the disc version.
 */
char *disc_get_version (disc *d, u_int8_t *v, char **v_s) {
	if (v)
		*v = d -> version;
	
	if (v_s)
		*v_s = d -> version_string;

	return (*v_s);
}


/**
 * Retrieves the disc game title.
 * @param d The disc structure.
 * @param t_s This will point to a string describing the disc title.
 * @return A string describing the disc title.
 */
char *disc_get_title (disc *d, char **t_s) {
	if (t_s)
		*t_s = d -> title;

	return (*t_s);
}


/**
 * Retrieves if the disc has an update.
 * @param d The disc structure.
 * @return True if the disc contains an update, false otherwise.
 */
bool disc_get_update (disc *d) {
	return (d -> has_update);
}


/**
 * Retrieves the number of sectors of the disc.
 * @param d The disc structure.
 * @return The number of sectors.
 */
u_int32_t disc_get_sectors_no (disc *d) {
	return (d -> sectors_no);
}

u_int32_t disc_get_layerbreak (disc *d) {
	return (d -> layerbreak);
}

u_int32_t disc_get_command (disc *d) {
	return (d -> command);
}

u_int32_t disc_get_method (disc *d) {
	return (d -> read_method);
}

u_int32_t disc_get_def_method (disc *d) {
	return dvd_get_def_method(d -> dvd);//(d -> def_read_method);
}

u_int32_t disc_get_sec_disc (disc *d) {
	return (d -> sec_disc);
}

u_int32_t disc_get_sec_mem (disc *d) {
	return (d -> sec_mem);
}

/* wiidevel@stacktic.org */
static bool disc_check_update (disc *d) {
	u_int8_t *buf;
	u_int32_t x;
	bool unscramble_old;

	if (d -> type == DISC_TYPE_WII || d -> type == DISC_TYPE_WII_DL) {
		/* Force unscrambling for this read */
		unscramble_old = d -> unscrambling;
		disc_set_unscrambling (d, true);

		/* We need to read offset 0x50004 of the disc. Sector 160 has offset 0x50000 */
		if (disc_read_sector (d, 160, &buf, NULL)) {
			x = my_ntohl (*(u_int32_t *) (buf + 4));
			if (x == 0xA5BED6AE)
				d -> has_update = false;
			else
				d -> has_update = true;
		} else {
			error ("disc_check_update() failed");
		}

		disc_set_unscrambling (d, unscramble_old);
	} else {
		/* GameCube discs never have an update, as actually the GC firmware cannot be upgrade */
		d -> has_update = false;
	}

	return (d -> has_update);
}


/**
 * Sets the disc read method.
 * @param d The disc structure.
 * @param method The requested method.
 * @return True if the method was set correctly, false otherwise (i. e.: method too small/big).
 */
bool disc_set_read_method (disc *d, int method) {
	bool out;
	u_int32_t deviation;
	u_int32_t counter;
	u_int32_t cnt1;

	d -> command = dvd_get_command(d -> dvd);
//	d -> def_read_method = dvd_get_def_method(d -> dvd);
	d -> read_method = method;

	out = true;
	switch (method) {
		case 0:
			d -> read_sector = disc_read_sector_0;
			break;
		case 1:
			d -> read_sector = disc_read_sector_1;
			break;
		case 2:
			d -> read_sector = disc_read_sector_2;
			break;
		case 3:
			d -> read_sector = disc_read_sector_3;
			break;
		case 4:
			d -> read_sector = disc_read_sector_4;
			break;
		case 5:
			d -> read_sector = disc_read_sector_5;
			break;
		case 6:
			d -> read_sector = disc_read_sector_6;
			break;
		case 7:
			d -> read_sector = disc_read_sector_7;
			break;
		case 8:
			d -> read_sector = disc_read_sector_8;
			break;
		case 9:
			d -> read_sector = disc_read_sector_9;
			break;
		default:
			switch (dvd_get_def_method(d -> dvd)) {
			case 0: 
				d -> read_method = 0;
				d -> read_sector = disc_read_sector_0;
				break;
			case 1: 
				d -> read_method = 1;
				d -> read_sector = disc_read_sector_1;
				break;
			case 2: 
				d -> read_method = 2;
				d -> read_sector = disc_read_sector_2;
				break;
			case 3: 
				d -> read_method = 3;
				d -> read_sector = disc_read_sector_3;
				break;
			case 4: 
				d -> read_method = 4;
				d -> read_sector = disc_read_sector_4;
				break;
			case 5: 
				d -> read_method = 5;
				d -> read_sector = disc_read_sector_5;
				break;
			case 6: 
				d -> read_method = 6;
				d -> read_sector = disc_read_sector_6;
				break;
			case 7: 
				d -> read_method = 7;
				d -> read_sector = disc_read_sector_7;
				break;
			case 8: 
				d -> read_method = 8;
				d -> read_sector = disc_read_sector_8;
				break;
			case 9: 
				d -> read_method = 9;
				d -> read_sector = disc_read_sector_9;
				break;
			default:
				d -> read_method = DEFAULT_READ_METHOD;
				d -> read_sector = DEFAULT_READ_SECTOR;
				break;
			}
	}

	if (d->sec_disc==-1) {
		if ((d->read_method == 4) || (d->read_method == 5) || (d->read_method == 6)) 
			d->sec_disc=27;
		else 
			d->sec_disc=16;
	}
	if (d->sec_mem==-1) {
		if ((d->read_method == 4) || (d->read_method == 5) || (d->read_method == 6)) 
			d->sec_mem=27;
		else
			d->sec_mem=16;
	}

	deviation = d->sec_mem % SECTORS_PER_BLOCK;
	counter=0;

	if (deviation>3) {
		cnt1=deviation;
		while (1==1) {
			cnt1+=deviation;
			counter++;
			if (cnt1%SECTORS_PER_BLOCK<=1) break;
		}
	}
	d -> max_cnt = counter;
	d -> max_blk = ((d->sec_mem*(d->max_cnt+1))-((d->sec_mem*(d->max_cnt+1)) % SECTORS_PER_BLOCK)) / 16;

	if (out) {
		debug ("Read method set to %d", d -> read_method);
	} else {
		error ("Cannot set read method\n");
	}

	return (out);
}


/**
 * Controls the unscrambling process.
 * @param d The disc structure.
 * @param unscramble If true, every raw sectors read will be unscrambled to check if they are error-free, otherwise read data will be returned as-is.
 */
void disc_set_unscrambling (disc *d, bool unscramble) {
	d -> unscrambling = unscramble;
	debug ("Sectors unscrambling %s", unscramble ? "enabled" : "disabled");

	return;
}


static void disc_crack_seeds (disc *d) {
	int i;

	/* As a Nintendo GameCube/Wii disc should not have too many keys, 20 should be enough */
	debug ("Retrieving all DVD seeds");
	for (i = 0; i < 20 * 16; i += 16)
		disc_read_sector (d, i, NULL, NULL);

	return;
}


/**
 * Creates a new structure representing a Nintendo GameCube/Wii optical disc.
 * @param dvd_device The CD/DVD-ROM device, in OS-dependent format (i.e.: /dev/something on Unix, x: on Windows).
 * @return The newly-created structure, to be used with the other commands.
 */
disc *disc_new (char *dvd_device, u_int32_t command) {
	dvd_drive *dvd;
	disc *d;

	if ((dvd = dvd_drive_new (dvd_device, command))) {
		d = (disc *) malloc (sizeof (disc));
		memset (d, 0, sizeof (disc));
		d -> dvd = dvd;
		d -> u = unscrambler_new ();
		disc_set_unscrambling (d, true);	// Unscramble by default
		disc_set_read_method (d, DEFAULT_READ_METHOD);
		disc_cache_init (d, DISC_DEFAULT_CACHE_SIZE);
	} else {
		d = NULL;
	}

	return (d);
}


bool disc_init (disc *d, u_int32_t disctype, u_int32_t sectors_no) {
	bool out;
	
	d -> sectors_no = 1000; 		// TODO
	disc_detect_type (d, disctype, sectors_no);
	disc_crack_seeds (d);
//	unscrambler_set_bruteforce (d -> u, false);		// Disabling bruteforcing will allow us to detect errors more quickly
	unscrambler_set_bruteforce (d -> u, true);
	if (d -> type==DISC_TYPE_DVD) {
		my_strdup (d -> title, "DVD"+'\0');
		out = true;
	}
	else if (disc_analyze (d)) {
		disc_check_update (d);
		out = true;
	} else {
		out = false;
	}

	return (out);
}


/**
 * Frees resources used by a disc structure and destroys it.
 * @param d The disc structure.
 * @return NULL.
 */
void *disc_destroy (disc *d) {
	disc_cache_destroy (d);
	unscrambler_destroy (d -> u);
	my_free (d -> version_string);
	my_free (d -> title);
	dvd_drive_destroy (d -> dvd);
	my_free (d);

	return (NULL);
}


char *disc_get_drive_model_string (disc *d) {
	return (dvd_get_model_string (d -> dvd));
}


bool disc_get_drive_support_status (disc *d) {
	return (dvd_get_support_status (d -> dvd));
}

void disc_set_speed (disc *d, u_int32_t speed) {
	if (speed != -1) dvd_set_speed (d -> dvd, speed, NULL);
}

void disc_set_streaming_speed (disc *d, u_int32_t speed) {
	if (speed != -1) dvd_set_streaming (d -> dvd, speed, NULL);
}

bool disc_stop_unit (disc *d, bool start) {
	if (dvd_stop_unit (d -> dvd, start, NULL) == 0) return true;
	else return false;
}

void init_range (disc *d, u_int32_t sec_disc, u_int32_t sec_mem) {
	if ((sec_disc>=1)&&(sec_disc<=100)) d->sec_disc = sec_disc;
	else d->sec_disc = -1;
	if ((sec_mem>=16)&&(sec_mem<=100)) d->sec_mem = sec_mem;
	else d->sec_mem = -1;
}