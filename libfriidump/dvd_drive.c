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
 * \brief A class to send raw MMC commands to a CD/DVD-ROM drive.
 *
 * This class can be used to send raw MMC commands to a CD/DVD-ROM drive. It uses own structures and data types to represent the commands, which are
 * then transformed in the proper OS-dependent structures when the command is executed, achieving portability. Currently Linux and Windows are supported, but
 * all that is needed to add support to a new OS is a proper <code>dvd_execute_cmd()</code> function, so it should be very easy. I hope that someone can add
 * compatibility with MacOS X and *BSD: libcdio is a good place to understand how it should be done :). Actally, we could have used libcdio right from the start,
 * but I didn't want to add a dependency on a library that cannot be easily found in binary format for all the target OS's.
 *
 * This file contains code derived from the work of Kevin East (SeventhSon), kev@kev.nu, http://www.kev.nu/360/ , which, in turn, derives from work by
 * a lot of other people. See his page for full details.
 */

#include "rs.h"
#include "misc.h"
#include <stdio.h>
#include <sys/types.h>
//#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "dvd_drive.h"
#include "disc.h"

#ifdef WIN32
#include <windows.h>
#include <ntddscsi.h>
#else
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif


/*! \brief Timeout for MMC commands.
 *
 * This must be expressed in seconds (Windows uses seconds, right?).
 */
#define MMC_CMD_TIMEOUT 10


/* Imported drive-specific functions */
int vanilla_2064_dvd_dump_mem	(dvd_drive *dvd, u_int32_t block_off, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf);
int vanilla_2384_dvd_dump_mem	(dvd_drive *dvd, u_int32_t block_off, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf);
int hitachi_dvd_dump_mem	(dvd_drive *dvd, u_int32_t block_off, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf);
int liteon_dvd_dump_mem		(dvd_drive *dvd, u_int32_t block_off, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf);
int renesas_dvd_dump_mem	(dvd_drive *dvd, u_int32_t block_off, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf);


/*! \brief A structure that represents a CD/DVD-ROM drive.
 */
struct dvd_drive_s {
	/* Device special file */
	char *device;			//!< The path to the drive (i.e.: /dev/something on Unix, x: on Windows).

	/* Data about the drive */
	char *vendor;			//!< The drive vendor.
	char *prod_id;			//!< The drive product ID.
	char *prod_rev;			//!< The drive product revision (Usually firmware version).
	char *model_string;		//!< The above three strings, joined in a single one.
	u_int32_t def_method;
	u_int32_t command;

	/* Device-dependent internal memory dump function */
	/*! The intended area should start where sector data is stored upon a READ command. Here we assume that sectors are
	 *  stored one after the other, as heuristics showed it is the case for the Hitachi MN103-based drives, but this model
	 *  might be changed in the future, if we get support for other drives.
	 */
	dvd_drive_memdump_func memdump;	//!< A pointer to a function that is able to dump the drive's internal memory area.
	bool supported;			//!< True if the drive is a supported model, false otherwise.


	/* File descriptor & stuff used to access drive */
#ifdef WIN32
	HANDLE fd;			//!< The HANDLE to interact with the drive on Windows.
#else
	int fd;				//!< The file descriptor to interact with the drive on Unix.
#endif
};


/** \brief Supported MMC commands.
 */
enum mmc_commands_e {
	SPC_INQUIRY = 0x12,
	MMC_READ_12 = 0xA8,
};


/**
 * Initializes a structure representing an MMC command.
 * @param mmc A pointer to the MMC command structure.
 * @param buf The buffer where results of the MMC command execution provided by the drive should be stored, or NULL if no buffer will be provided.
 * @param len The length of the buffer (ignored in case buf is NULL).
 * @param sense A pointer to a structure which will hold the SENSE DATA got from the drive after the command has been executed, or NULL.
 */
void dvd_init_command (mmc_command *mmc, u_int8_t *buf, int len, req_sense *sense) {
	memset (mmc, 0, sizeof (mmc_command));
	if (buf)
		memset (buf, 0, len);
	mmc -> buffer = buf;
	mmc -> buflen = buf ? len : 0;
	mmc -> sense = sense;
	
	return;
}


#ifdef WIN32

/* Doc is under the UNIX function */
int dvd_execute_cmd (dvd_drive *dvd, mmc_command *mmc, bool ignore_errors) {
	SCSI_PASS_THROUGH_DIRECT *sptd;
	unsigned char sptd_sense[sizeof (*sptd) + 18], *sense;
	DWORD bytes;
	int out;

	sptd = (SCSI_PASS_THROUGH_DIRECT *) sptd_sense;
	sense = &sptd_sense[sizeof (*sptd)];
	
	memset (sptd, 0, sizeof (sptd_sense));
	memcpy (sptd -> Cdb, mmc -> cmd, sizeof (mmc -> cmd));
	sptd -> Length = sizeof (SCSI_PASS_THROUGH);
	sptd -> CdbLength = 12;
	sptd -> SenseInfoLength = 18;
	sptd -> DataIn = SCSI_IOCTL_DATA_IN;
	sptd -> DataBuffer = mmc -> buffer;
	// Quick hack: Windows hates sptd->DataTransferLength = 1, so we set it to 2 and ignore the second byte.
	if (mmc -> buflen == 1)		// TODO
		sptd -> DataTransferLength = 2;
	else
		sptd -> DataTransferLength = mmc -> buflen;
	sptd -> TimeOutValue = MMC_CMD_TIMEOUT;
	sptd -> SenseInfoOffset = sizeof (*sptd);

	//fprintf (stdout,"mmc->cmd[00] = %d \n",mmc->cmd[00]);
	//Set streaming hack
	if (mmc->cmd[00]==0xB6) {
		sptd -> DataIn = SCSI_IOCTL_DATA_OUT;
		sptd -> DataTransferLength = 28;
	}

	if (!DeviceIoControl (dvd -> fd, IOCTL_SCSI_PASS_THROUGH_DIRECT, sptd, sizeof (*sptd) + 18, sptd, sizeof (*sptd) + 18, &bytes, NULL) && !ignore_errors) {
		out = -1;	/* Failure */
		error ("Execution of MMC command failed: %s", strerror (errno));
// 		error ("DeviceIOControl() failed with %d\n", GetLastError());
		debug ("Command was: ");
		hex_and_ascii_print ("", mmc -> cmd, sizeof (mmc -> cmd));
		debug ("Sense data: %02X/%02X/%02X\n", sense[2] & 0x0F, sense[12], sense[13]);
	} else {
		out = 0;
	}
	
	if (mmc -> sense) {
		mmc -> sense -> sense_key = sense[2];
		mmc -> sense -> asc = sense[12];
		mmc -> sense -> ascq = sense[13];
	}
	
	return (out);
}

#else

/**
 * Executes an MMC command.
 * @param dvd The DVD drive the command should be exectued on.
 * @param mmc The command to be executed.
 * @param ignore_errors If set to true, no error will be printed if the command fails.
 * @return 0 if the command was executed successfully, < 0 otherwise.
 */
int dvd_execute_cmd (dvd_drive *dvd, mmc_command *mmc, bool ignore_errors) {
	int out;
	struct cdrom_generic_command cgc;
	struct request_sense sense;
	
#if 0
	debug ("Executing MMC command: ");
	hex_and_ascii_print ("", mmc -> cmd, sizeof (mmc -> cmd));
#endif

	/* Init Linux-format MMC command */
	memset (&cgc, 0, sizeof (struct cdrom_generic_command));
	memcpy (cgc.cmd, mmc -> cmd, sizeof (mmc -> cmd));
	cgc.buffer = (unsigned char *) mmc -> buffer;
	cgc.buflen = mmc -> buflen;
	cgc.data_direction = CGC_DATA_READ;
	cgc.timeout = MMC_CMD_TIMEOUT * 1000;	/* Linux uses milliseconds */
	cgc.sense = &sense;
	if (ioctl (dvd -> fd, CDROM_SEND_PACKET, &cgc) < 0 && !ignore_errors) {
		out = -1;	/* Failure */
		error ("Execution of MMC command failed: %s", strerror (errno));
		debug ("Command was:");
		hex_and_ascii_print ("", cgc.cmd, sizeof (cgc.cmd));
		debug ("Sense data: %02X/%02X/%02X", sense.sense_key, sense.asc, sense.ascq);
	} else {
		out = 0;
	}
	
	if (mmc -> sense) {
		mmc -> sense -> sense_key = sense.sense_key;
		mmc -> sense -> asc = sense.asc;
		mmc -> sense -> ascq = sense.ascq;
	}
	
	return (out);
}
#endif


/**
 * Sends an INQUIRY command to the drive to retrieve drive identification strings.
 * @param dvd The DVD drive the command should be exectued on.
 * @return 0 if the command was executed successfully, < 0 otherwise.
 */
static int dvd_get_drive_info (dvd_drive *dvd) {
	mmc_command mmc;
	int out;
	u_int8_t buf[36];
	char tmp[36 * 4];
	
	dvd_init_command (&mmc, buf, sizeof (buf), NULL);
	mmc.cmd[0] = SPC_INQUIRY;
	mmc.cmd[4] = sizeof (buf);
	if ((out = dvd_execute_cmd (dvd, &mmc, false)) >= 0) {
		my_strndup (dvd -> vendor, buf + 8, 8);
		strtrimr (dvd -> vendor);
		my_strndup (dvd -> prod_id, buf + 16, 16);
		strtrimr (dvd -> prod_id);
		my_strndup (dvd -> prod_rev, buf + 32, 4);
		strtrimr (dvd -> prod_rev);
		snprintf (tmp, sizeof (tmp), "%s/%s/%s", dvd -> vendor, dvd -> prod_id, dvd -> prod_rev);
		my_strdup (dvd -> model_string, tmp);
		
		debug ("DVD drive is \"%s\"", dvd -> model_string);
	} else {
		error ("Cannot identify DVD drive\n");
	}

	return (out);
}


/**
 * Assigns the proper memory dump functions to a dvd_drive object, according to vendor, model and other parameters. Actually this scheme probably needs to
 * to be improved, but it is enough for the moment.
 * @param dvd The DVD drive the command should be exectued on.
 */
static void dvd_assign_functions (dvd_drive *dvd, u_int32_t command) {
	dvd -> def_method = 0;
	if (strcmp (dvd -> vendor, "HL-DT-ST") == 0 && (
//		strcmp (dvd -> prod_id, "DVDRAM GSA-T10N") == 0 ||
		strcmp (dvd -> prod_id, "DVD-ROM GDR8082N") == 0 ||
		strcmp (dvd -> prod_id, "DVD-ROM GDR8161B") == 0 ||
		strcmp (dvd -> prod_id, "DVD-ROM GDR8162B") == 0 ||
		strcmp (dvd -> prod_id, "DVD-ROM GDR8163B") == 0 || 
		strcmp (dvd -> prod_id, "DVD-ROM GDR8164B") == 0
	)) {
		debug ("Hitachi MN103-based DVD drive detected, using Hitachi memory dump command");
		dvd -> memdump = &hitachi_dvd_dump_mem;
		dvd -> command = 2;
		dvd -> supported = true;
		dvd -> def_method = 9;

	} else if (strcmp (dvd -> vendor, "LITE-ON") == 0 && (
		strcmp (dvd ->prod_id, "DVDRW LH-18A1H") == 0 ||
		strcmp (dvd ->prod_id, "DVDRW LH-18A1P") == 0 ||
		strcmp (dvd ->prod_id, "DVDRW LH-20A1H") == 0 ||
		strcmp (dvd ->prod_id, "DVDRW LH-20A1P") == 0
	)) {
		debug ("Lite-On DVD drive detected, using Lite-On memory dump command");
		dvd -> memdump = &liteon_dvd_dump_mem;
		dvd -> command = 3;
		dvd -> supported = true;
		dvd -> def_method = 5;

	} else if (strcmp (dvd -> vendor, "TSSTcorp") == 0 && (
		strcmp (dvd ->prod_id, "DVD-ROM SH-D162A") == 0 ||
		strcmp (dvd ->prod_id, "DVD-ROM SH-D162B") == 0 ||
		strcmp (dvd ->prod_id, "DVD-ROM SH-D162C") == 0 ||
		strcmp (dvd ->prod_id, "DVD-ROM SH-D162D") == 0
	)) {
		debug ("Toshiba Samsung DVD drive detected, using vanilla 2384 memory dump command");
		dvd -> memdump = &vanilla_2384_dvd_dump_mem;
		dvd -> command = 1;
		dvd -> supported = true;
		dvd -> def_method = 0;

	} else if (strcmp (dvd -> vendor, "PLEXTOR") == 0) {
		debug ("Plextor DVD drive detected, using vanilla 2064 memory dump command");
		dvd -> memdump = &vanilla_2064_dvd_dump_mem;
		dvd -> command = 0;
		dvd -> supported = true;
		dvd -> def_method = 2;

	} else {
		/* This is an unsupported drive (yet). */
		dvd -> memdump = &vanilla_2064_dvd_dump_mem;
		dvd -> command = 0;
		dvd -> supported = false;
	}

	if (command!=-1) {
		dvd -> command = command;
		if	    (command == 0) dvd -> memdump = &vanilla_2064_dvd_dump_mem;
		else if	(command == 1) dvd -> memdump = &vanilla_2384_dvd_dump_mem;
		else if	(command == 2) dvd -> memdump = &hitachi_dvd_dump_mem;
		else if	(command == 3) dvd -> memdump = &liteon_dvd_dump_mem;
		else if	(command == 4) dvd -> memdump = &renesas_dvd_dump_mem;
	}

	//init Reed-Solomon for Lite-On
	generate_gf();
	gen_poly();

	return;
}


/**
 * Creates a new structure representing a CD/DVD-ROM drive.
 * @param device The CD/DVD-ROM device, in OS-dependent format (i.e.: /dev/something on Unix, x: on Windows).
 * @return The newly-created structure, to be used with the other commands, or NULL if the drive could not be initialized.
 */
dvd_drive *dvd_drive_new (char *device, u_int32_t command) {
	dvd_drive *dvd;
#ifdef WIN32
	HANDLE fd;
	char dev[40];
#else
	int fd;
#endif

	/* Force the dropping of privileges: in our model, privileges are only used to execute memory dump commands, the user
	   must gain access to the device somehow else (i. e. get added to the "cdrom" group or similar things) */
	drop_euid ();
	
	debug ("Trying to open DVD device %s", device);
#ifdef WIN32
	sprintf (dev, "\\\\.\\%c:", device[0]);
	if ((fd = CreateFile (dev, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		error ("Cannot open drive: %d", GetLastError ());
#else
	if ((fd = open (device, O_RDONLY | O_NONBLOCK)) < 0) {
		perror ("Cannot open drive");
#endif
		dvd = NULL;
	} else {
		debug ("Opened successfully");
		drop_euid ();
		dvd = (dvd_drive *) malloc (sizeof (dvd_drive));
		my_strdup (dvd -> device, device);
		dvd -> fd = fd;
		dvd_get_drive_info (dvd);
		dvd_assign_functions (dvd, command);
	}

	return (dvd);
}


/**
 * Frees resources used by a DVD drive structure and destroys it.
 * @param dvd The DVD drive structure to be destroyed.
 * @return NULL.
 */
void *dvd_drive_destroy (dvd_drive *dvd) {
	if (dvd) {
#ifdef WIN32
		CloseHandle (dvd -> fd);
#else
		close (dvd -> fd);
#endif
		my_free (dvd -> device);
		my_free (dvd -> vendor);
		my_free (dvd -> prod_id);
		my_free (dvd -> prod_rev);
		my_free (dvd);
	}

	return (NULL);
}


/**
 * Executes the drive-dependent function to dump the drive sector cache, and returns the dumped data.
 * @param dvd The DVD drive the command should be exectued on.
 * @param block_off The offset to start dumping, WRT the beginning of the sector cache.
 * @param block_len The number of blocks to dump.
 * @param block_size The block size to be used for dumping.
 * @param buf A buffer where to store the dumped data. Note that this must be able to hold at least block_len * block_size bytes.
 * @return 0 if the command was executed successfully, < 0 otherwise.
 */
int dvd_memdump (dvd_drive *dvd, u_int32_t block_off, u_int32_t block_len, u_int32_t block_size, u_int8_t *buf) {
	int out;

	/* Upgrade privileges and call actual dump functions */
	upgrade_euid ();
	out = dvd -> memdump (dvd, block_off, block_len, block_size, buf);
	drop_euid ();

	return (out);
}


/**
 * Issues a READ(12) command without bothering to return the results. Uses the FUA (Force Unit Access bit) so that the requested sectors are actually read
 * at the beginning of the cache and can be dumped later.
 * @param dvd The DVD drive the command should be exectued on.
 * @param sector The sector to be read. What will be cached is the 16-sectors block to which the sector belongs.
 * @param sense A pointer to a structure which will hold the SENSE DATA got from the drive after the command has been executed.
 * @return 0 if the command was executed successfully, < 0 otherwise.
 */
int dvd_read_sector_dummy (dvd_drive *dvd, u_int32_t sector, u_int32_t sectors, req_sense *sense, u_int8_t *extbuf, size_t extbufsize) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[64 * 1024], *buf;
	size_t bufsize;

	/* We need some buffer, be it provided externally or not */
	if (extbuf) {
		buf = extbuf;
		bufsize = extbufsize;
	} else {
		buf = intbuf;
		bufsize = sizeof (intbuf);
	}

	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = MMC_READ_12;
	mmc.cmd[1] = 0x08;	/* FUA bit set */
	mmc.cmd[2] = (u_int8_t) ((sector & 0xFF000000) >> 24);	/* LBA from MSB to LSB */
	mmc.cmd[3] = (u_int8_t) ((sector & 0x00FF0000) >> 16);
	mmc.cmd[4] = (u_int8_t) ((sector & 0x0000FF00) >> 8);
	mmc.cmd[5] = (u_int8_t)  (sector & 0x000000FF);
	mmc.cmd[6] = (u_int8_t) ((sectors & 0xFF000000) >> 24);	/* Size from MSB to LSB */
	mmc.cmd[7] = (u_int8_t) ((sectors & 0x00FF0000) >> 16);
	mmc.cmd[8] = (u_int8_t) ((sectors & 0x0000FF00) >> 8);
	mmc.cmd[9] = (u_int8_t)  (sectors & 0x000000FF);
	out = dvd_execute_cmd (dvd, &mmc, true);		/* Ignore errors! */

	return (out);
}


/**
 * Issues a READ(12) command using the STREAMING bit, which causes the requested 16-sector block to be read into memory,
 * together with the following four. This way we will be able to dump 5 sector with a single READ request.
 *
 * Note the strange need for a big buffer even though we must only pass 0x10 as the transfer length, otherwise the drive will hang (!?).
 * @param dvd The DVD drive the command should be exectued on.
 * @param sector The sector to be read. What will be cached is the 16-sectors block to which the sector belongs, and the following 4 blocks.
 * @param sense A pointer to a structure which will hold the SENSE DATA got from the drive after the command has been executed.
 * @param extbuf A buffer where to store the read data, or NULL.
 * @param extbufsize The size of the buffer.
 * @return 
 */
int dvd_read_sector_streaming (dvd_drive *dvd, u_int32_t sector, req_sense *sense, u_int8_t *extbuf, size_t extbufsize) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[2048 * 16], *buf;
	size_t bufsize;

	/* We need some buffer, be it provided externally or not */
	if (extbuf) {
		buf = extbuf;
		bufsize = extbufsize;
	} else {
		buf = intbuf;
		bufsize = sizeof (intbuf);
	}
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = MMC_READ_12;
	mmc.cmd[2] = (u_int8_t) ((sector & 0xFF000000) >> 24);	/* LBA from MSB to LSB */
	mmc.cmd[3] = (u_int8_t) ((sector & 0x00FF0000) >> 16);
	mmc.cmd[4] = (u_int8_t) ((sector & 0x0000FF00) >> 8);
	mmc.cmd[5] = (u_int8_t) (sector & 0x000000FF);
	mmc.cmd[6] = 0;
	mmc.cmd[7] = 0;
	mmc.cmd[8] = 0;
	mmc.cmd[9] = 0x10;
	mmc.cmd[10] = 0x80;	/* STREAMING bit set */
	out = dvd_execute_cmd (dvd, &mmc, true);		/* Ignore errors! */
	
	return (out);
}


int dvd_read_streaming (dvd_drive *dvd, u_int32_t sector, u_int32_t sectors, req_sense *sense, u_int8_t *extbuf, size_t extbufsize) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[64 * 1024], *buf;
	size_t bufsize;

	/* We need some buffer, be it provided externally or not */
	if (extbuf) {
		buf = extbuf;
		bufsize = extbufsize;
	} else {
		buf = intbuf;
		bufsize = sizeof (intbuf);
	}
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = MMC_READ_12;
	mmc.cmd[2] = (u_int8_t) ((sector & 0xFF000000) >> 24);	/* LBA from MSB to LSB */
	mmc.cmd[3] = (u_int8_t) ((sector & 0x00FF0000) >> 16);
	mmc.cmd[4] = (u_int8_t) ((sector & 0x0000FF00) >> 8);
	mmc.cmd[5] = (u_int8_t)  (sector & 0x000000FF);
	mmc.cmd[6] = (u_int8_t) ((sectors & 0xFF000000) >> 24);	/* Size from MSB to LSB */
	mmc.cmd[7] = (u_int8_t) ((sectors & 0x00FF0000) >> 16);
	mmc.cmd[8] = (u_int8_t) ((sectors & 0x0000FF00) >> 8);
	mmc.cmd[9] = (u_int8_t)  (sectors & 0x000000FF);
	mmc.cmd[10] = 0x80;	/* STREAMING bit set */
	out = dvd_execute_cmd (dvd, &mmc, true);		/* Ignore errors! */
	
	return (out);
}


int dvd_flush_cache_READ12 (dvd_drive *dvd, u_int32_t sector, req_sense *sense) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[64], *buf;
	size_t bufsize;

	buf = intbuf;
	bufsize = 0;
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = MMC_READ_12;
	mmc.cmd[1] = 0x08;
	mmc.cmd[2] = (u_int8_t) ((sector & 0xFF000000) >> 24);	/* LBA from MSB to LSB */
	mmc.cmd[3] = (u_int8_t) ((sector & 0x00FF0000) >> 16);
	mmc.cmd[4] = (u_int8_t) ((sector & 0x0000FF00) >> 8);
	mmc.cmd[5] = (u_int8_t)  (sector & 0x000000FF);
	out = dvd_execute_cmd (dvd, &mmc, true);
	
	return (out);
}

int dvd_stop_unit (dvd_drive *dvd, bool start, req_sense *sense) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[64], *buf;
	size_t bufsize;

	buf = intbuf;
	bufsize = 0;
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = 0x1B;
	if (start) mmc.cmd[4] = 1;
	else mmc.cmd[4] = 0;
	out = dvd_execute_cmd (dvd, &mmc, true);
	
	return (out);
}

int dvd_set_speed (dvd_drive *dvd, u_int32_t speed, req_sense *sense) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[64], *buf;
	size_t bufsize;

	buf = intbuf;
	bufsize = 0;
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = 0xBB;
	mmc.cmd[2] = (u_int8_t) ((speed & 0x0000FF00) >> 8);
	mmc.cmd[3] = (u_int8_t)  (speed & 0x000000FF);
	out = dvd_execute_cmd (dvd, &mmc, true);
	
	return (out);
}

int dvd_get_size (dvd_drive *dvd, u_int32_t *size, req_sense *sense) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[64], *buf;
	size_t bufsize;

	buf = intbuf;
	bufsize = 0x22;
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = 0x52;
	mmc.cmd[1] = 0x01;
	mmc.cmd[5] = 0x01;
	mmc.cmd[8] = 0x22;
	out = dvd_execute_cmd (dvd, &mmc, true);

	*(size)=*(size) << 8 | intbuf[0x18];
	*(size)=*(size) << 8 | intbuf[0x19];
	*(size)=*(size) << 8 | intbuf[0x1a];
	*(size)=*(size) << 8 | intbuf[0x1b];

	return (out);
}

int dvd_get_layerbreak (dvd_drive *dvd, u_int32_t *layerbreak, req_sense *sense) {
	mmc_command mmc;
	int out;
	u_int8_t intbuf[2052], *buf;
	size_t bufsize;

	buf = intbuf;
	bufsize = 2052;
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[0] = 0xad;
	mmc.cmd[8] = 0x08;
	mmc.cmd[9] = 0x04;
	out = dvd_execute_cmd (dvd, &mmc, true);

	*(layerbreak)=*(layerbreak) << 8;
	*(layerbreak)=*(layerbreak) << 8 | intbuf[0x11];
	*(layerbreak)=*(layerbreak) << 8 | intbuf[0x12];
	*(layerbreak)=*(layerbreak) << 8 | intbuf[0x13];
	if (*(layerbreak) > 0) *(layerbreak)=*(layerbreak) - 0x30000 + 1;

	return (out);
}

int dvd_set_streaming (dvd_drive *dvd, u_int32_t speed, req_sense *sense) {
/*
DVD Decrypter->
DeviceIoControl    : \Device\CdRom5
Command            : IOCTL_SCSI_PASS_THROUGH_DIRECT
Length             : 44 (0x002C)
ScsiStatus         : 0
PathId             : 0
TargedId           : 0
Lun                : 0
CdbLength          : 12 (0x0C)
SenseInfoLength    : 24 (0x18)
DataTransferLength : 28 (0x0000001C)
DataIn             : 0
TimeOutValue       : 5000

CDB:
00000000  B6 00 00 00 00 00 00 00 00 00 1C 00               ¶...........    

Data Sent:
00000000  00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF   ............____
00000010  00 00 03 E8 FF FF FF FF 00 00 03 E8               ...è____...è    
*/
	mmc_command mmc;
	int out;
	u_int8_t inbuf[28], *buf;
	size_t bufsize;

	buf = inbuf;
	bufsize = 28;
	
	dvd_init_command (&mmc, buf, bufsize, sense);
	mmc.cmd[00] = 0xB6;
	mmc.cmd[10] = 28;

	*(buf+ 0)=0;//2
	*(buf+ 1)=0;
	*(buf+ 2)=0;
	*(buf+ 3)=0;
	*(buf+ 4)=0; //MSB
	*(buf+ 5)=0; //
	*(buf+ 6)=0; //
	*(buf+ 7)=0; //LSB

	*(buf+ 8)=0xff; //MSB
	*(buf+ 9)=0xff; //
	*(buf+10)=0xff; //
	*(buf+11)=0xff; //LSB

	*(buf+12)=(u_int8_t) ((speed & 0xFF000000) >> 24);
	*(buf+13)=(u_int8_t) ((speed & 0x00FF0000) >> 16);
	*(buf+14)=(u_int8_t) ((speed & 0x0000FF00) >> 8);
	*(buf+15)=(u_int8_t)  (speed & 0x000000FF);

	*(buf+16)=(u_int8_t) ((1000 & 0xFF000000) >> 24);
	*(buf+17)=(u_int8_t) ((1000 & 0x00FF0000) >> 16);
	*(buf+18)=(u_int8_t) ((1000 & 0x0000FF00) >> 8);
	*(buf+19)=(u_int8_t)  (1000 & 0x000000FF);

	*(buf+20)=(u_int8_t) ((speed & 0xFF000000) >> 24);
	*(buf+21)=(u_int8_t) ((speed & 0x00FF0000) >> 16);
	*(buf+22)=(u_int8_t) ((speed & 0x0000FF00) >> 8);
	*(buf+23)=(u_int8_t)  (speed & 0x000000FF);

	*(buf+24)=(u_int8_t) ((1000 & 0xFF000000) >> 24);
	*(buf+25)=(u_int8_t) ((1000 & 0x00FF0000) >> 16);
	*(buf+26)=(u_int8_t) ((1000 & 0x0000FF00) >> 8);
	*(buf+27)=(u_int8_t)  (1000 & 0x000000FF);

	out = dvd_execute_cmd (dvd, &mmc, true);

	return (out);
}


char *dvd_get_vendor (dvd_drive *dvd) {
	return (dvd -> vendor);
}


char *dvd_get_product_id (dvd_drive *dvd) {
	return (dvd -> prod_id);
}


char *dvd_get_product_revision (dvd_drive *dvd) {
	return (dvd -> prod_rev);
}


char *dvd_get_model_string (dvd_drive *dvd) {
	return (dvd -> model_string);
}


bool dvd_get_support_status (dvd_drive *dvd) {
	return (dvd -> supported);
}

u_int32_t dvd_get_def_method (dvd_drive *dvd){
	return (dvd -> def_method);
}

u_int32_t dvd_get_command (dvd_drive *dvd){
	return (dvd -> command);
}