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
 * \brief Unscrambler for Nintendo GameCube/Wii discs.
 *
 * As Nintendo GameCube/Wii discs use the standars DVD-ROM scrambling algorithm, but with different, unknown, seeds, the actual seeds have to be brute-forced.
 * The functions in this file take care of the brute-forcing and of the actual unscrambling of the read sectors.
 *
 * The code in this file has been derived from unscrambler 0.4, Copyright (C) 2006 Victor Muñoz (xt5@ingenieria-inversa.cl), GPL v2+,
 * http://www.ingenieria-inversa.cl/?lp_lang_pref=en .
 */

#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "constants.h"
#include "byteorder.h"
#include "ecma-267.h"
#include "unscrambler.h"

// #define unscramblerdebug(...) debug (__VA_ARGS__);
#define unscramblerdebug(...)

/*! \brief Size of the seeds cache (Do not touch) */
#define MAX_SEEDS 4

/*! \brief Number of bytes of a sector on which the EDC is calculated */
#define EDC_LENGTH (RAW_SECTOR_SIZE - 4)		/* The EDC value is contained in the bottom 4 bytes of a frame */

u_int8_t disctype;
/*! \brief A structure that represents a seed
 */
typedef struct t_seed {
    int seed;						//!< The seed, in numeric format.
    unsigned char streamcipher[SECTOR_SIZE];		//!< The stream cipher generated from the seed through the LFSR.
} t_seed;


/*! \brief A structure that represents an unscrambler
 */
struct unscrambler_s {
	t_seed seeds[(MAX_SEEDS + 1) * 16];		//!< The seeds cache.
	bool bruteforce_seeds;				//!< If true, whenever a seed for a sector is not cached, it will be found via a bruteforce attack, otherwise an error will be returned.
};

void unscrambler_set_disctype (u_int8_t disc_type){
	disctype = disc_type;
//	fprintf (stdout,"%d",disctype);
}

/**
 * Adds a seed to the cache, calculating its streamcipher.
 * @param seeds The seed cache.
 * @param seed The seed to add.
 * @return A structure representing the added seed, or NULL if it could not be added.
 */
static t_seed *add_seed (t_seed *seeds, unsigned short seed) {
	int i;
	t_seed *out;

 	unscramblerdebug ("Caching seed %04x\n", seed);

	if (seeds -> seed == -2) {
		out = NULL;
	} else {
		seeds -> seed = seed;

		LFSR_init (seed);
		for (i = 0; i < SECTOR_SIZE; i++)
			seeds -> streamcipher[i] = LFSR_byte ();

		out = seeds;
	}

	return (out);
}


/**
 * Tests if the specified seed is the one used for the specified sector block: the check is done comparing the generated EDC with the one at the bottom of each
 * sector. Sectors are processed in blocks, as the same seed is used for 16 consecutive sectors.
 * @param buf The sector.
 * @param j The seed.
 * @return true if the seed is correct, false otherwise.
 */
static bool test_seed (u_int8_t *buf, int j) {
	int i;
	u_int8_t tmp[RAW_SECTOR_SIZE];
	u_int32_t edc_calculated, edc_correct;
	bool out;

	memcpy (tmp, buf, RAW_SECTOR_SIZE);

	LFSR_init (j);
	for (i = 12; i < EDC_LENGTH; i++)
		tmp[i] ^= LFSR_byte ();

	edc_calculated = edc_calc (0x00000000, tmp, EDC_LENGTH);
	edc_correct = my_ntohl (*((u_int32_t *) (&tmp[EDC_LENGTH])));
	if (edc_calculated == edc_correct)
		out = true;
	else
		out = false;

	return (out);
}


/**
 * Unscramble a complete block, using an already-cached seed.
 * @param seed The seed to use for the unscrambling.
 * @param _bin The 16-sector block to unscramble (RAW_BLOCK_SIZE).
 * @param _bout The unscrambled 16-sector block (BLOCK_SIZE).
 * @return True if the unscrambling was successful, false otherwise.
 */
static bool unscramble_frame (t_seed *seed, u_int8_t *_bin, u_int8_t *_bout) {
	int i, j;
	u_int8_t tmp[RAW_SECTOR_SIZE], *bin, *bout;
	u_int32_t *_4bin, *_4cipher, edc_calculated, edc_correct;
	bool out;

	out = true;
	for(j = 0; j < 16; j++) {
		bin = &_bin[RAW_SECTOR_SIZE * j];
		bout = &_bout[SECTOR_SIZE * j];

		memcpy (tmp, bin, RAW_SECTOR_SIZE);
		_4bin = (u_int32_t *) &tmp[12];		/* Scrambled data begin at byte 12 */
		_4cipher = (u_int32_t *) seed -> streamcipher;
		for (i = 0; i < 512; i++)		/* Well, the scrambling algorithm is just a bitwise XOR... */
			_4bin[i] ^= _4cipher[i];

		//memcpy (bout, tmp + 6, SECTOR_SIZE); // copy CPR_MAI bytes

		if (disctype==3) { //Regular
			memcpy (bout, tmp + 12, SECTOR_SIZE); // DVD: copy 2048 bytes (starting from CPR_MAI)
		}
		else { //Nintendo
			memcpy (bout, tmp + 6, SECTOR_SIZE);  // Nintendo: copy 2048 bytes (up to CPR_MAI)
			memcpy (&_bin[(RAW_SECTOR_SIZE * j)+2054], &tmp[2054], 6);
		}

		edc_calculated = edc_calc (0x00000000, tmp, EDC_LENGTH);
		edc_correct = my_ntohl (*((u_int32_t *) (&tmp[EDC_LENGTH])));
		if (edc_calculated != edc_correct) {
			debug ("Bad EDC (%08x), must be %08x (sector = %d)", edc_calculated, edc_correct, j);
			out = false;
		}
	}

	return (out);
}


/**
 * Initializes the seed cache.
 * @param u The unscrambler structure.
 */
static void unscrambler_init_seeds (unscrambler *u) {
	int i, j;

	for (i = 0; i < 16; i++) {
		for (j = 0; j < MAX_SEEDS; j++)
			u -> seeds[i * MAX_SEEDS + j].seed = -1;

		u -> seeds[i * MAX_SEEDS + j].seed = -2;	// TODO Check what this does
	}

	return;
}


/**
 * Creates a new structure representing an unscrambler.
 * @return The newly-created structure, to be used with the other commands.
 */
unscrambler *unscrambler_new (void) {
	unscrambler *u;

	u = (unscrambler *) malloc (sizeof (unscrambler));
	unscrambler_init_seeds (u);
	u -> bruteforce_seeds = true;

	return (u);
}


/**
 * Frees resources used by an unscrambler structure and destroys it.
 * @param u The unscrambler structure.
 * @return NULL.
 */
void *unscrambler_destroy (unscrambler *u) {
	my_free (u);

	return (NULL);
}


void unscrambler_set_bruteforce (unscrambler *u, bool b) {
	u -> bruteforce_seeds = b;
	debug ("Seed bruteforcing %s", b ? "enabled" : "disabled");

	return;
}


/**
 * Unscrambles a 16-sector block.
 * @param u The unscrambler structure.
 * @param sector_no The number of the first sector in the block.
 * @param inbuf The 16-sector block to unscramble. Each block must be RAW_SECTOR_SIZE bytes long, so that the total size is RAW_BLOCK_SIZE.
 * @param outbuf The unscrambled 16-sector block. Each block will be SECTOR_SIZE bytes long, so that the total size is BLOCK_SIZE.
 * @return True if the unscrambling was successful, false otherwise.
 */
bool unscrambler_unscramble_16sectors (unscrambler *u, u_int32_t sector_no, u_int8_t *inbuf, u_int8_t *outbuf) {
	t_seed *seeds;
	t_seed *current_seed;
	int j;
	bool out;

	out = true;

	seeds = &(u -> seeds[((sector_no / 16) & 0x0F) * MAX_SEEDS]);

	/* Try to find the seed used for this sector */
	current_seed = NULL;
	while (!current_seed && (seeds -> seed) >= 0) {
		if (test_seed (inbuf, seeds -> seed))
			current_seed = seeds;
		else
			seeds++;
	}

	if (!current_seed && u -> bruteforce_seeds) {
		/* The seed is not cached, yet. Try to find it with brute force... */
		unscramblerdebug ("Brute-forcing seed for sector %d...", sector_no);

		for (j = 0; !current_seed && j < 0x7FFF; j++) {
			if (test_seed (inbuf, j)) {
				if (!(current_seed = add_seed (seeds, j))) {
					error ("No enough cache space for caching seed");
					out = false;
				}
			}
		}

		if (current_seed)
			unscramblerdebug ("Seed found: %04x", --j);
	}

	if (current_seed) {
		/* OK, somehow seed was found: unscramble frame, write it and go on */
		if (!unscramble_frame (current_seed, inbuf, outbuf)) {
			error ("Error unscrambling frame %u\n", sector_no);
			out = false;
		} else {
			out = true;
		}
	} else {
		/* Well, we only get here if there are read errors */
		error ("Cannot find seed for frame %u", sector_no);
		out = false;
	}

	return (out);
}


/**
 * Unscrambles a complete file.
 * @param u The unscrambler structure.
 * @param infile The input file name.
 * @param outfile The output file name.
 * @param progress A function to be called repeatedly during the operation, useful to report progress data/statistics.
 * @param progress_data Data to be passed as-is to the progress function.
 * @return True if the unscrambling was successful, false otherwise.
 */
bool unscrambler_unscramble_file (unscrambler *u, char *infile, char *outfile, unscrambler_progress_func progress, void *progress_data, u_int32_t *current_sector) {
	FILE *in, *outfp;
	bool out;
	u_int8_t b_in[RAW_BLOCK_SIZE], b_out[BLOCK_SIZE];
	size_t r;
	my_off_t filesize;
	int s;
	u_int32_t total_sectors;

	out = false;
	if(!(in = fopen (infile ? infile : "", "rb"))) {
		error ("Cannot open input file \"%s\"", infile);
	} else if (!(outfp = fopen (outfile ? outfile : "", "wb"))) {
		error ("Cannot open output file \"%s\"", outfile);
		fclose (in);
	} else {
		/* Find out how many sectors we need to process */
		my_fseek (in, 0, SEEK_END);
		filesize = my_ftell (in);
		total_sectors = (u_int32_t) (filesize / RAW_SECTOR_SIZE);
		rewind (in);

		/* First call to progress function */
		if (progress)
			progress (true, 0, total_sectors, progress_data);

		s = 0, out = true;
		while ((r = fread (b_in, 1, RAW_BLOCK_SIZE, in)) > 0 && out) {
			if (r < RAW_BLOCK_SIZE) {
				warning ("Short block read (%u bytes), padding with zeroes!", r);
				memset (b_in + r, 0, sizeof (b_in) - r);
			}

			if (unscrambler_unscramble_16sectors (u, s, b_in, b_out)) {
				clearerr (outfp);

				if (!b_out) {
					error ("NULL buffer");
					out = false;
					*(current_sector) = s;
				}
				else fwrite (b_out, SECTOR_SIZE, SECTORS_PER_BLOCK, outfp);
				if (ferror (outfp)) {
					error ("fwrite() to ISO output file failed");
					out = false;
					*(current_sector) = s;
				}
			} else {
				debug ("unscrambler_unscramble_16sectors() failed");
				out = false;
				*(current_sector) = s;
			}

			s += 16;

			if ((s % 320 == 0) || (s == total_sectors)) { //speedhack
				if (progress)
					progress (false, s, total_sectors, progress_data);
			}
		}

		if (out) {
			debug ("Image successfully unscrambled");
		}

		fclose (in);
		fclose (outfp);
	}

	return (out);
}
