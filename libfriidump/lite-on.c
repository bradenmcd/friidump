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
 * Command found in Lite-On LH-18A1H; verified with LH-18A1P and LH-20A1H
 * Expected to work with: DH*, DW*, LH*, SH* series
 * Possibly would work with other Mediatek based drives:
 * Samsung SE*, SH* series
 * Some Dell drives
 * Some Sony drives
 * Asus DRW-1814* (DRW* series?)
 */

//u_int8_t  tmp[64*1024];

/**
 * @param dvd The DVD drive the command should be exectued on.
 * @param offset The absolute memory offset to start dumping.
 * @param block_size The block size for the dump.
 * @param buf Where to place the dumped data. This must have been setup by the caller to store up to block_size bytes.
 * @return < 0 if an error occurred, 0 otherwise.
 */
int liteon_dvd_dump_memblock (dvd_drive *dvd, u_int32_t offset, u_int32_t block_size, u_int8_t *buf) {
	mmc_command mmc;
	int out;
	u_int32_t raw_block_size;
	u_int32_t raw_offset;

	u_int32_t src_offset;
	u_int32_t dst_offset;
	u_int32_t row_nr;
	u_int32_t sec_nr;
	u_int32_t sec_cnt;
	u_int32_t first_sec_nr;
	u_int8_t  tmp[64*1024];
	//u_int8_t  *tmp;

	raw_block_size = (block_size / 2064) * 0x950;
	raw_offset = (offset / 2064) * 0x950;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else if (!block_size || raw_block_size >= 65535) {
		error ("invalid raw_block_size (valid: 1 - 65534)");
		error ("raw_block_size = (block_size / 2064) * 2384");
		out = -2;
	} else {
		//tmp = malloc(64*1024);
		dvd_init_command (&mmc, tmp, raw_block_size, NULL); //64*1024
		mmc.cmd[0]  = 0x3C; // READ BUFFER
		mmc.cmd[1]  = 0x01; // Vendor specific - sole parameter supported by Lite-On
		mmc.cmd[2]  = 0x01; // == 0x02; 0xE2 = EEPROM; 0xF1 = KEYPARA;
		mmc.cmd[3]  = (unsigned char) ((raw_offset & 0x00FF0000) >> 16);// address MSB
		mmc.cmd[4]  = (unsigned char) ((raw_offset & 0x0000FF00) >> 8);	// address
		mmc.cmd[5]  = (unsigned char) ( raw_offset & 0x000000FF);	// address LSB
		mmc.cmd[6]  = (unsigned char) ((raw_block_size & 0x00FF0000) >> 16);	// length  MSB
		mmc.cmd[7]  = (unsigned char) ((raw_block_size & 0x0000FF00) >> 8);	// length
		mmc.cmd[8]  = (unsigned char) ( raw_block_size & 0x000000FF);		// length  LSB

		out = dvd_execute_cmd (dvd, &mmc, false);

		src_offset   = 0;
		dst_offset   = 0;
		first_sec_nr = 0x55555555;
		sec_cnt      = 0;
		while (src_offset < raw_block_size) {
			sec_nr=(*(tmp+src_offset+1)<<16)+(*(tmp+src_offset+2)<<8)+(*(tmp+src_offset+3));
//fprintf (stdout,"sec_nr: %x\n", sec_nr);
			if (first_sec_nr==0x55555555) {
				first_sec_nr=sec_nr;
				rs_decode(tmp+src_offset, 0, 0);
			}
			else
				if (sec_nr==first_sec_nr+sec_cnt) {
					rs_decode(tmp+src_offset, 0, 0); //sector seq = ok
				}
				else { //sector seq broken -> corrupt
					error ("sector sequence broken");
					out = -3;
					*(tmp+src_offset+0)=0xff;
					*(tmp+src_offset+1)=0xff;
					*(tmp+src_offset+2)=0xff;
					*(tmp+src_offset+3)=0xff;
					break;
				}
			for (row_nr=0; row_nr<12; row_nr++) {
				memcpy(buf+dst_offset, tmp+src_offset, 172);
				dst_offset += 172;
				src_offset += 182;
			}
			src_offset += 200;
			sec_cnt += 1;
		}
		//free(tmp);
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
int liteon_dvd_dump_mem (dvd_drive *dvd, u_int32_t offset, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf) {
	u_int32_t i;
	int r, out;

	if (!buf) {
		error ("NULL buffer");
		out = -1;
	} else {
		r = -10;
		for (i = 0; i < block_len; i++) {
			if ((r = liteon_dvd_dump_memblock (dvd, offset + i * block_size, block_size, buf + i * block_size)) < 0) {
				error ("liteon_dvd_dump_memblock() failed with %d", r);
				break;
			}
		}
		out = r;
	}

	return (out);
}
