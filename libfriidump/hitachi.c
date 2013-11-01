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
 * \brief Memory dump functions specific to drives based on the Hitachi MN103 chip.
 *
 * Drives which are supported by this set of functions are, for instance, the LG GDR-8161B, GDR-8162B, GDR-8163B and GDR-8164B. All testing has been performed
 * with the latter model, so I'm not really sure about the others, but they should work ;). Please report any issues and more compatible drives!
 *
 * This file contains code derived from the work of Kevin East (SeventhSon), kev@kev.nu, http://www.kev.nu/360/ , which, in turn, derives from work by
 * a lot of other people. See his page for full details.
 */

#include <stdio.h>
#include <sys/types.h>
#include "misc.h"
#include "dvd_drive.h"

/*! \brief Memory offset at which the drive cache memory is mapped.
 *
 * During READ, sector data is cached beginning at this address.
 */

#define HITACHI_MEM_BASE 0x80000000


/**
 * Dumps a single block (with arbitrary size) of the address space of the MN103 microcontroller within the Hitachi-LG Xbox 360 DVD drive and similar drives.
 * This function is derived from the work of Kevin East (SeventhSon), kev@kev.nu, http://www.kev.nu/360/.
 * @param dvd The DVD drive the command should be exectued on.
 * @param offset The absolute memory offset to start dumping.
 * @param block_size The block size for the dump.
 * @param buf Where to place the dumped data. This must have been setup by the caller to store up to block_size bytes.
 * @return < 0 if an error occurred, 0 otherwise.
 */
int hitachi_dvd_dump_memblock (dvd_drive *dvd, u_int32_t offset, u_int32_t block_size, u_int8_t *buf) {
	mmc_command mmc;
	int out;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else if (!block_size || block_size > 65535) {
		error ("invalid block_size (valid: 1 - 65535)");
		out = -2;
	} else {
		dvd_init_command (&mmc, buf, block_size, NULL);
		mmc.cmd[0] = 0xE7; // vendor specific command (discovered by DaveX)
		mmc.cmd[1] = 0x48; // H
		mmc.cmd[2] = 0x49; // I
		mmc.cmd[3] = 0x54; // T
		mmc.cmd[4] = 0x01; // read MCU memory sub-command
		mmc.cmd[6] = (unsigned char) ((offset & 0xFF000000) >> 24);	// address MSB
		mmc.cmd[7] = (unsigned char) ((offset & 0x00FF0000) >> 16);	// address
		mmc.cmd[8] = (unsigned char) ((offset & 0x0000FF00) >> 8);	// address
		mmc.cmd[9] = (unsigned char) (offset & 0x000000FF);		// address LSB
		mmc.cmd[10] = (unsigned char) ((block_size & 0xFF00) >> 8);	// length MSB
		mmc.cmd[11] = (unsigned char) (block_size & 0x00FF);		// length LSB

		out = dvd_execute_cmd (dvd, &mmc, false);
	}

	return (out);
}


/**
 * Dumps a given portion of the address space of the MN103 microcontroller within the Hitachi-LG Xbox 360 DVD drive.
 * WARNING: it can take a while to dump a lot of data.
 * @param dvd The DVD drive the command should be exectued on.
 * @param offset The absolute memory offset to start dumping.
 * @param block_len The number of blocks to dump.
 * @param block_size The block size for the dump.
 * @param buf Where to place the dumped data. This must have been setup by the caller to store up to block_size bytes.
 * @return < 0 if an error occurred, 0 otherwise.
 */
int hitachi_dvd_dump_mem_generic (dvd_drive *dvd, u_int32_t offset, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf) {
	u_int32_t i;
	int r, out;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else {
		r = -10;
		for (i = 0; i < block_len; i++) {
			if ((r = hitachi_dvd_dump_memblock (dvd, offset + i * block_size, block_size, buf + i * block_size)) < 0) {
				error ("hitachi_dvd_read_block() failed with %d", r);
				break;
			}
		}
		out = r;
	}

	return (out);
}


/**
 * Dumps a given portion of the address space of the MN103 microcontroller within the Hitachi-LG Xbox 360 DVD drive, starting at the offset at which
 * sector data are cached.
 * WARNING: it can take a while to dump a lot of data.
 * @param dvd The DVD drive the command should be exectued on.
 * @param offset The memory offset to start dumping, relative to the cache start offset.
 * @param block_len The number of blocks to dump.
 * @param block_size The block size for the dump.
 * @param buf Where to place the dumped data. This must have been setup by the caller to store up to block_size bytes.
 * @return < 0 if an error occurred, 0 otherwise.
 */
int hitachi_dvd_dump_mem (dvd_drive *dvd, u_int32_t offset, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf) {
	u_int32_t i;
	int r, out;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else {
		r = -10;
		for (i = 0; i < block_len; i++) {
			if ((r = hitachi_dvd_dump_memblock (dvd, HITACHI_MEM_BASE + offset + i * block_size, block_size, buf + i * block_size)) < 0) {
				error ("hitachi_dvd_read_block() failed with %d", r);
				break;
			}
		}
		out = r;
	}

	return (out);
}
