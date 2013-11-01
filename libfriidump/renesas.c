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

#include "rs.h"
#include <stdio.h>
#include <sys/types.h>
#include "misc.h"
#include "dvd_drive.h"

/**
 * Command found in Lite-On LH-18A1H
 * Expected to work with: DH*, DW*, LH*, SH* series
 * Possibly would work with other Mediatek based drives:
 * Samsung SE*, SH* series
 * Some Dell drives
 * Some Sony drives
 * Asus DRW-1814* (DRW* series?)
 */


/**
 * @param dvd The DVD drive the command should be exectued on.
 * @param offset The absolute memory offset to start dumping.
 * @param block_size The block size for the dump.
 * @param buf Where to place the dumped data. This must have been setup by the caller to store up to block_size bytes.
 * @return < 0 if an error occurred, 0 otherwise.
 */
int renesas_dvd_dump_memblock (dvd_drive *dvd, u_int32_t offset, u_int32_t block_size, u_int8_t *buf) {
	mmc_command mmc;
	int out;

	u_int32_t src_offset;
	u_int32_t sec_nr;
	u_int32_t sec_cnt;
	u_int32_t first_sec_nr;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else if (!block_size || block_size > 65535) {
		error ("invalid block_size (valid: 1 - 65535)");
		out = -2;
	} else {
		dvd_init_command (&mmc, buf, block_size, NULL);
		mmc.cmd[0] = 0x3C;
		mmc.cmd[1] = 0x05;
		mmc.cmd[2]  = (unsigned char) ((offset & 0xFF000000) >> 24);	// address MSB
		mmc.cmd[3]  = (unsigned char) ((offset & 0x00FF0000) >> 16);	// address MSB
		mmc.cmd[4]  = (unsigned char) ((offset & 0x0000FF00) >> 8);	// address
		mmc.cmd[5]  = (unsigned char) ( offset & 0x000000FF);		// address LSB
//		mmc.cmd[6]  = (unsigned char) ((block_size & 0x00FF0000) >> 16);// length  MSB
		mmc.cmd[6]  = 0;
		mmc.cmd[7]  = (unsigned char) ((block_size & 0x0000FF00) >> 8);	// length
		mmc.cmd[8]  = (unsigned char) ( block_size & 0x000000FF);	// length  LSB
		mmc.cmd[9]  = 0x44;

		out = dvd_execute_cmd (dvd, &mmc, false);

		src_offset   = 0;
		first_sec_nr = 0x55555555;
		sec_cnt      = 0;
		while (src_offset < block_size) {
			sec_nr=(*(buf+src_offset+1)<<16)+(*(buf+src_offset+2)<<8)+(*(buf+src_offset+3));
			if (first_sec_nr==0x55555555) {
				first_sec_nr=sec_nr;
			}
			else
				if (sec_nr!=first_sec_nr+sec_cnt) {
					error ("sector sequence broken");
					out = -3;
					*(buf+src_offset+0)=0xff;
					*(buf+src_offset+1)=0xff;
					*(buf+src_offset+2)=0xff;
					*(buf+src_offset+3)=0xff;
					break;
				}
			src_offset += 2064;
			sec_cnt += 1;
		}
	}
	return (out);
}



/**
 * WARNING: it can take a while to dump a lot of data.
 * @param dvd The DVD drive the command should be exectued on.
 * @param offset The memory offset to start dumping, relative to the cache start offset.
 * @param block_len The number of blocks to dump.
 * @param block_size The block size for the dump.
 * @param buf Where to place the dumped data. This must have been setup by the caller to store up to block_size bytes.
 * @return < 0 if an error occurred, 0 otherwise.
 */
int renesas_dvd_dump_mem (dvd_drive *dvd, u_int32_t offset, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf) {
	u_int32_t i;
	int r, out;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else {
		r = -10;
		for (i = 0; i < block_len; i++) {
			if ((r = renesas_dvd_dump_memblock (dvd, offset + i * block_size, block_size, buf + i * block_size)) < 0) {
				error ("renesas_dvd_dump_memblock() failed with %d", r);
				break;
			}
		}
		out = r;
	}

	return (out);
}
