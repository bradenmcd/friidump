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

#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "disc.h"
#include "dumper.h"
#include "unscrambler.h"

#define USECS_PER_SEC	1000000


/* Name of package */
#define PACKAGE "friidump"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "arep@no.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "FriiDump"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.5.3.1"


#ifdef WIN32

#include "getopt-win32.h"


#else
#include <sys/time.h>
#include <getopt.h>
#endif


/* Struct for program options */
struct {
	char *device;
	bool autodump;
	bool gui;
	char *raw_in;
	char *raw_out;
	char *iso_out;
	bool resume;
	int dump_method;
	u_int32_t command;
	u_int32_t start_sector;
	u_int32_t sectors_no;
	u_int32_t speed;
	u_int32_t disctype;
	u_int32_t sec_disc;
	u_int32_t sec_mem;
	bool no_hashing;
	bool no_unscrambling;
	bool no_flushing;
	bool stop_unit;
	bool allmethods;
} options;


/* Struct for progress data */
typedef struct {
	struct timeval start_time;
	struct timeval end_time;
	double mb_total;
	double mb_total_real;
	u_int32_t sectors_skipped;
} progstats;


void progress_for_guis (bool start, u_int32_t sectors_done, u_int32_t total_sectors, progstats *stats) {
	int perc;
	double elapsed, mb_done, mb_done_real, mb_hour, seconds_left;
	struct timeval now;
	time_t eta;
	struct tm etatm;
	char buf[50];
	
	if (start) {
		gettimeofday (&(stats -> start_time), NULL);
		stats -> mb_total = (double) total_sectors * 2064 / 1024 / 1024;
		stats -> mb_total_real = (double) (total_sectors - sectors_done) * 2064 / 1024 / 1024;
		stats -> sectors_skipped = sectors_done;
	} else {
		perc = (int) (100.0 * sectors_done / total_sectors);
		gettimeofday (&now, NULL);
		elapsed = difftime (now.tv_sec, (stats -> start_time).tv_sec);
		mb_done = (double) sectors_done * 2064 / 1024 / 1024;
		mb_done_real = (double) (sectors_done - stats -> sectors_skipped) * 2064 / 1024 / 1024;
		mb_hour = mb_done_real / elapsed * 60 * 60;
		seconds_left = stats -> mb_total_real / mb_hour * 60 * 60;
		eta = (time_t) ((stats -> start_time).tv_sec + seconds_left);
		if (localtime_r (&eta, &etatm))
			strftime (buf, 50, "%d/%m/%Y %H:%M:%S", &etatm);
		else
			sprintf (buf, "N/A");

		/* This is the only thing we print to stdout, so that other programs can easily capture and parse our output */
		fprintf (stdout, "%d%%|%u/%u sectors|%.2lf/%.0lf MB|%.0lf/%.0lf seconds|%.2lf MB/h|%s\n",
			 perc, sectors_done, total_sectors, mb_done, stats -> mb_total, elapsed, seconds_left, mb_hour, buf);
		fflush (stdout);
	}

	/* Save return time, in case this will be the last call */
	gettimeofday (&(stats -> end_time), NULL);

	return;
}


void progress (bool start, u_int32_t sectors_done, u_int32_t total_sectors, progstats *stats) {
	int perc, i;
	double elapsed, mb_done, mb_done_real, mb_hour, seconds_left;
	struct timeval now;
	time_t eta;
	struct tm etatm;
	char buf[50];
	
	if (start) {
		gettimeofday (&(stats -> start_time), NULL);
		stats -> mb_total = (double) total_sectors * 2064 / 1024 / 1024;
		stats -> mb_total_real = (double) (total_sectors - sectors_done) * 2064 / 1024 / 1024;
		stats -> sectors_skipped = sectors_done;
	} else {
		perc = (int) (100.0 * sectors_done / total_sectors);
		gettimeofday (&now, NULL);
		elapsed = difftime (now.tv_sec, (stats -> start_time).tv_sec);
		mb_done = (double) sectors_done * 2064 / 1024 / 1024;
		mb_done_real = (double) (sectors_done - stats -> sectors_skipped) * 2064 / 1024 / 1024;
		mb_hour = mb_done_real / elapsed * 60 * 60;
		seconds_left = stats -> mb_total_real / mb_hour * 60 * 60;
		eta = (time_t) ((stats -> start_time).tv_sec + seconds_left);
		if (localtime_r (&eta, &etatm))
			strftime (buf, 50, "%d/%m/%Y %H:%M:%S", &etatm);
		else
			sprintf (buf, "N/A");

		fprintf (stdout, "\r%3d%% ", perc);
		fprintf (stdout, "|");
		for (i = 0; i < 100 / 3; i++) {
			if (i == perc / 3)
				fprintf (stdout, "*");
			else
				fprintf (stdout, "-");
		}
		fprintf (stdout, "| ");
		fprintf (stdout, "%.2lf MB/h, ETA: %s", mb_hour, buf);
		fflush (stdout);
	}

	if (sectors_done == total_sectors)
		printf ("\n");

	/* Save return time, in case this will be the last call */
	gettimeofday (&(stats -> end_time), NULL);

	return;
}



void welcome (void) {
	/* Welcome text */
	fprintf (stderr,
		"FriiDump " PACKAGE_VERSION " - Copyright (C) 2007 Arep\n"
		"This software comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; see COPYING for details.\n"
		"\n"
		"Official support forum: http://wii.console-tribe.com\n"
		"\n"
		"Forum for this UNOFFICIAL VERSION: http://forum.redump.org\n"
		"\n"
		);
	fflush (stderr);

	return;
}


void help (void) {
	/* 80 cols guide:
	 *      |-------------------------------------------------------------------------------|
	 */
	fprintf (stderr, "\n"
		"Available command line options:\n"
		"\n"
		" -h, --help			Show this help\n"
		" -a, --autodump			Dump the disc to an ISO file with an\n"
		"				automatically-generated name, resuming the dump\n"
		"				if possible\n"
		" -g, --gui			Use more verbose output that can be easily\n"
		"				parsed by a GUI frontend\n"
		" -d, --device <device>		Dump disc from device <device>\n"
		" -p, --stop			Instruct device to stop disc rotation\n"
		" -c, --command <nr>		Force memory dump command:\n"
		"				0 - vanilla 2064\n"
		"				1 - vanilla 2384\n"
		"				2 - Hitachi\n"
		"				3 - Lite-On\n"
		"				4 - Renesas\n"
		" -x, --speed <x>		Set streaming speed (1, 24, 32, 64, etc.,\n"
		"				where 1 = 150 KiB/s and so on)\n"
		" -T, --type <nr>		Force disc type:\n"
		"				0 - GameCube\n"
		"				1 - Wii\n"
		"				2 - Wii_DL\n"
		"				3 - DVD\n"
		" -S, --size <sectors>		Force disc size\n"
		" -r, --raw <file>		Output to file <file> in raw format (2064-byte\n"
		"				sectors)\n"
		" -i, --iso <file>		Output to file <file> in ISO format (2048-byte\n"
		"				sectors)\n"
		" -u, --unscramble <file>	Convert (unscramble) raw image contained in\n"
		"				<file> to ISO format\n"
		" -H, --nohash			Do not compute CRC32/MD5/SHA-1 hashes\n"
		"				for generated files\n"
		" -s, --resume			Resume partial dump\n"
		"				-  General  -----------------------------------\n"
		" -0, --method0[=<req>,<exp>]	Use dumping method 0 (Optional argument\n"
		"				specifies how many sectors to request from disc\n"
		"				and read from cache at a time. Values should be\n"
		"				separated with a comma. Default 16,16)\n"
		"				-  Non-Streaming  -----------------------------\n"
		" -1, --method1[=<req>,<exp>]	Use dumping method 1 (Default 16,16)\n"
		" -2, --method2[=<req>,<exp>]	Use dumping method 2 (Default 16,16)\n"
		" -3, --method3[=<req>,<exp>]	Use dumping method 3 (Default 16,16)\n"
		"				-  Streaming  ---------------------------------\n"
		" -4, --method4[=<req>,<exp>]	Use dumping method 4 (Default 27,27)\n"
		" -5, --method5[=<req>,<exp>]	Use dumping method 5 (Default 27,27)\n"
		" -6, --method6[=<req>,<exp>]	Use dumping method 6 (Default 27,27)\n"
		"				-  Hitachi  -----------------------------------\n"
		" -7, --method7			Use dumping method 7 (Read and dump 5 blocks\n"
		"				at a time, using streaming read)\n"
		" -8, --method8			Use dumping method 8 (Read and dump 5 blocks\n"
		"				at a time, using streaming read, using DMA)\n"
		" -9, --method9			Use dumping method 9 (Read and dump 5 blocks\n"
		"				at a time, using streaming read, using DMA and\n"
		"				some speed tricks)\n"
		" -A, --allmethods		Try all known methods and commands until\n"
		"				one works.\n"
#ifdef DEBUG
		" -n, --donottunscramble		Do not try unscrambling to check EDC. Only\n"
		"				useful for testing the raw performance of the\n"
		"				different methods\n"
		" -f, --donottflush		Do not call fflush() after every fwrite()\n"
#endif
	);

	return;
}


bool optparse (int argc, char **argv) {
	bool out;
	char *result = NULL;
	int c;
	int option_index = 0;
	static struct option long_options[] = {
		{"help", 0, 0, 'h'},	//0 - no_argument
		{"autodump", 0, 0, 'a'},
		{"gui", 0, 0, 'g'},
		{"device", 1, 0, 'd'},	//1 - required_argument
		{"raw", 1, 0, 'r'},
		{"iso", 1, 0, 'i'},
		{"unscramble", 1, 0, 'u'},
		{"nohash", 0, 0, 'H'},
		{"resume", 0, 0, 's'},
		{"method0", 2, 0, '0'},	//2 - optional_argument
		{"method1", 2, 0, '1'},
		{"method2", 2, 0, '2'},
		{"method3", 2, 0, '3'},
		{"method4", 2, 0, '4'},
		{"method5", 2, 0, '5'},
		{"method6", 2, 0, '6'},
		{"method7", 0, 0, '7'},
		{"method8", 0, 0, '8'},
		{"method9", 0, 0, '9'},
		{"stop", 0, 0, 'p'},
		{"command", 1, 0, 'c'},
		{"startsector", 1, 0, 't'},
		{"size", 1, 0, 'S'},
		{"speed", 1, 0, 'x'},
		{"type", 1, 0, 'T'},
		{"allmethods", 0, 0, 'A'},
#ifdef DEBUG
		/* We don't want newbies to generate and put into circulation bad dumps, so this options are disabled for releases */
		{"donottunscramble", 0, 0, 'n'},
		{"donottflush", 0, 0, 'f'},
#endif
		{0, 0, 0, 0}
	};

	if (argc == 1) {
		help ();
		exit (1);
	}
	
	/* Init options to default values */
	options.device = NULL;
	options.autodump = false;
	options.gui = false;
	options.raw_in = NULL;
	options.raw_out = NULL;
	options.iso_out = NULL;
	options.no_hashing = false;
	options.resume = false;
	options.dump_method = -1;
	options.command = -1;
	options.start_sector = -1;
	options.sectors_no = -1;
	options.speed = -1;
	options.disctype = -1;
	options.sec_disc = -1;
	options.sec_mem = -1;
	options.no_unscrambling = false;
	options.no_flushing = false;
	options.stop_unit = false;
	options.allmethods = false;

	do {
#ifdef DEBUG
		c = getopt_long (argc, argv, "hpagd:r:i:u:Hs0::1::2::3::4::5::6::789c:t:S:x:T:Anf", long_options, &option_index);
#else
		c = getopt_long (argc, argv, "hpagd:r:i:u:Hs0::1::2::3::4::5::6::789c:t:S:x:T:A", long_options, &option_index);
#endif

		switch (c) {
			case 'h':
				help ();
				exit (1);
				break;
			case 'p':
				options.stop_unit = true;
				break;
			case 'a':
				options.autodump = true;
				options.resume = true;
				break;
			case 'g':
				options.gui = true;
				break;
			case 'd':
				my_strdup (options.device, optarg);
				break;
			case 'r':
				my_strdup (options.raw_out, optarg);
				break;
			case 'i':
				my_strdup (options.iso_out, optarg);
				break;
			case 'u':
				my_strdup (options.raw_in, optarg);
				break;
			case 'H':
				options.no_hashing = true;
				break;
			case 's':
				options.resume = true;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
				options.dump_method = c - '0';
				if (optarg) {
					result = strtok(optarg, ",");
					result = strtok(NULL, ",");
					options.sec_disc = atol(strpbrk(optarg,"1234567890"));
					if (result) options.sec_mem = atol(result);
					else {
						help ();
						exit (1);
					}
				}
				break;
			case '7':
			case '8':
			case '9':
				options.dump_method = c - '0';
				break;
			case 'c':
				options.command = atol (optarg);
				if (options.command > 4) {
					help ();
					exit (1);
				};
				break;
			case 't':
				options.start_sector = atol (optarg);
				break;
			case 'S':
				options.sectors_no = atol (optarg);
				break;
			case 'x':
				options.speed = atol (optarg);
				break;
			case 'T':
				options.disctype = atol (optarg);
				if (options.disctype > 3) {
					help ();
					exit (1);
				};
				unscrambler_set_disctype (options.disctype);
				break;
			case 'A':
				options.allmethods = true;
				options.resume = true;
				break;
#ifdef DEBUG
			case 'n':
				options.no_unscrambling = true;
				break;
			case 'f':
				options.no_flushing = true;
				break;
#endif
			case -1:
				break;
			default:
// 				fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
				exit (7);
				break;
		}
	} while (c != -1);

	if (optind < argc) {
		/* Command-line arguments remaining. Ignore them, warning the user. */
		fprintf (stderr, "WARNING: Extra parameters ignored\n");
	}

	/* Sanity checks... */
	out = false;
	if (!options.device && !options.raw_in) {
		fprintf (stderr, "No operation specified. Please use the -d or -u options.\n");
	} else if (options.raw_in && options.raw_out) {
		fprintf (stderr,
			"Are you sure you want to convert a raw image to another raw image? ;)\n"
			"Take a look at the -i and -a options!\n"
		);
	} else if (options.autodump && (options.raw_out || options.iso_out)) {
		fprintf (stderr, "The -r and -i options cannot be used together with -a.\n");
	} else {
		/* Specified options seem to make sense */
		out = true;
	}
		
	return (out);
}

int dologic (disc *d, progstats stats) {
	disc_type type_id;
	char *type, *game_id, *region, *maker_id, *maker, *version, *title, tmp[0x03E0 + 4 + 1];
	bool drive_supported;
	int out;
	dumper *dmp;
	u_int32_t current_sector;
	
	

				if (options.stop_unit) { //stop rotation, if requested
					fprintf (stderr, "Issuing STOP command... %s\n", (disc_stop_unit (d, false)) ? "OK" : "Failed");
					exit (1);
				}
				else disc_stop_unit(d, true); //else start rotation

				drive_supported = disc_get_drive_support_status (d);
				fprintf (stderr,
					"\n"
					"Drive information:\n"
					"----------------------------------------------------------------------\n"
					"Drive model........: %s\n"
					"Supported..........: %s\n", disc_get_drive_model_string (d), drive_supported ? "Yes" : "No"
				);

				init_range(d, options.sec_disc, options.sec_mem);

				if (!(disc_set_read_method (d, options.dump_method)))
					exit (2);

				if (options.command!=-1) fprintf (stderr, 
					"Command............: %d (forced)\n", disc_get_command(d));
				else fprintf (stderr, 
					"Command............: %d\n", disc_get_command(d));
				options.dump_method=disc_get_method(d);
				if (disc_get_def_method(d)!=options.dump_method) fprintf (stderr, 
					"Method.............: %d (forced)\n", options.dump_method);
				else fprintf (stderr, 
					"Method.............: %d\n", options.dump_method);
				if ((options.dump_method==0) 
				|| (options.dump_method==1) || (options.dump_method==2) || (options.dump_method==3)
				|| (options.dump_method==4) || (options.dump_method==5) || (options.dump_method==6)
				){
					fprintf (stderr, 
					"Requested sectors..: %d\n", disc_get_sec_disc(d));
					fprintf (stderr, 
					"Expected sectors...: %d\n", disc_get_sec_mem(d));
				}

				fprintf (stderr, "\nPress Ctrl+C at any time to terminate\n");
				fprintf (stderr, "\nRetrieving disc seeds, this might take a while... ");

				//set speed for 1st time
				if (options.speed != -1) disc_set_speed(d, options.speed * 177);
				if (options.speed != -1) disc_set_streaming_speed(d, options.speed * 177);
//				disc_set_speed(d, 0xffff);

				if (!disc_init (d, options.disctype, options.sectors_no)) {
					fprintf (stderr, "Failed\n");
					out = false;
				} else {
					fprintf (stderr, "OK\n");
					disc_get_type (d, &type_id, &type);
					disc_get_gameid (d, &game_id);
					disc_get_region (d, NULL, &region);
					disc_get_maker (d, &maker_id, &maker);
					disc_get_version (d, NULL, &version);
					disc_get_title (d, &title);
					fprintf (stderr, 
						"\n"
						"Disc information:\n"
						"----------------------------------------------------------------------\n");

					if (options.disctype!=-1) fprintf (stderr, 
						"Disc type..........: %s (forced)\n", type);
					else fprintf (stderr, 
						"Disc type..........: %s\n", type);
					if (options.sectors_no!=-1) fprintf (stderr, 
						"Disc size..........: %d (forced)\n", disc_get_sectors_no(d));
					else fprintf (stderr, 
						"Disc size..........: %d\n", disc_get_sectors_no(d));

					if (disc_get_layerbreak(d)>0 && type_id==DISC_TYPE_DVD) fprintf (stderr, 
						"Layer break........: %d\n", disc_get_layerbreak(d));

					if ((type_id==DISC_TYPE_GAMECUBE) || (type_id==DISC_TYPE_WII) || (type_id==DISC_TYPE_WII_DL)) fprintf (stderr, 
						"Game ID............: %s\n"
						"Region.............: %s\n"
						"Maker..............: %s - %s\n"
						"Version............: %s\n"
						"Game title.........: %s\n", game_id, region, maker_id, maker, version, title
					);

					if (type_id == DISC_TYPE_WII || type_id == DISC_TYPE_WII_DL)
						fprintf (stderr, "Contains update....: %s\n" , disc_get_update (d) ? "Yes" : "No");
					fprintf (stderr, "\n");
					
					disc_set_unscrambling (d, !options.no_unscrambling);

					unscrambler_set_disctype (type_id);

					if (options.autodump) {
						snprintf (tmp, sizeof (tmp), "%s.iso", title);
						my_strdup (options.iso_out, tmp);
					}

					//set speed 2nd time after rotation is started and some sectors read
					if (options.speed != -1) disc_set_streaming_speed(d, options.speed * 177);
					if (options.speed != -1) disc_set_speed(d, options.speed * 177);

					/* If at least an output file was specified, proceed dumping, otherwise stop here */
					if (options.raw_out || options.iso_out) {
						if (options.raw_out)
							fprintf (stderr, "Writing to file \"%s\" in raw format\n", options.raw_out);
						if (options.iso_out)
							fprintf (stderr, "Writing to file \"%s\" in ISO format\n", options.iso_out);
						fprintf (stderr, "\n");

						dmp = dumper_new (d);

						dumper_set_hashing (dmp, !options.no_hashing);
						dumper_set_flushing (dmp, !options.no_flushing);

						if (!dumper_set_raw_output_file (dmp, options.raw_out, options.resume)) {
							fprintf (stderr, "Cannot setup raw output file\n");
						} else if (!dumper_set_iso_output_file (dmp, options.iso_out, options.resume)) {
							fprintf (stderr, "Cannot setup ISO output file\n");
						} else if (!dumper_prepare (dmp)) {
							fprintf (stderr, "Cannot prepare dumper");
						} else {
//	 						fprintf (stderr, "Starting dump process from sector %u...\n", dmp -> start_sector);
//		 					opdd.start_sector = options.start_sector;

							if (options.gui)
								dumper_set_progress_callback (dmp, (progress_func) progress_for_guis, &stats);
							else
								dumper_set_progress_callback (dmp, (progress_func) progress, &stats);

							if (dumper_dump (dmp, &current_sector)) {
								fprintf (stderr, "Dump completed successfully!\n");
								if (!options.no_hashing && options.raw_out)
									fprintf (stderr,
										"Raw image hashes:\n"
										"CRC32...: %s\n"
										//"MD4.....: %s\n"
										"MD5.....: %s\n"
										"SHA-1...: %s\n"
										/*"ED2K....: %s\n"*/,
										dumper_get_raw_crc32 (dmp), /*dumper_get_raw_md4 (dmp),*/ dumper_get_raw_md5 (dmp),
										dumper_get_raw_sha1 (dmp)/*, dumper_get_raw_ed2k (dmp)*/
									);
								if (!options.no_hashing && options.iso_out)
									fprintf (stderr,
										"ISO image hashes:\n"
										"CRC32...: %s\n"
										//"MD4.....: %s\n"
										"MD5.....: %s\n"
										"SHA-1...: %s\n"
										/*"ED2K....: %s\n"*/,
										dumper_get_iso_crc32 (dmp), /*dumper_get_iso_md4 (dmp),*/ dumper_get_iso_md5 (dmp),
										dumper_get_iso_sha1 (dmp)/*, dumper_get_iso_ed2k (dmp)*/
									);

								out = true;
								disc_stop_unit (d, 0);
							} else {
								fprintf (stderr, "\nDump failed at sectors: %u..%u\n", current_sector, current_sector+15);
								out = false;
								//disc_stop_unit (d, 0);
							}
						}

						dmp = dumper_destroy (dmp);
					} else {
						fprintf (stderr, "No output file for dumping specified, please take a look at the -i, -r and -a options\n");
					}
				}				
	return out;
}

int main (int argc, char *argv[]) {
	disc *d;
	progstats stats;
	double duration;
	suseconds_t us;
	int out, ret;
	unscrambler *u;
	unscrambler_progress_func pfunc;
	u_int32_t current_sector;

	/* First of all... */
	drop_euid ();
	
	welcome ();

	d = NULL;
	out = false;
	if (optparse (argc, argv)) {
		if (options.device) {
			/* Dump DVD to file */
			fprintf (stderr, "Initializing DVD drive... ");

			if (!(d = disc_new (options.device, options.command))) {
				fprintf (stderr, "Failed\n");
#ifndef WIN32
				fprintf (stderr,
					"Probably you do not have access to the DVD device. Ask the administrator\n"
					"to add you to the proper group, or use 'sudo'.\n"
				);
#endif
			} else {
			fprintf (stderr, "OK\n");
			
				if(options.allmethods)
				{
					fprintf (stderr, "Trying all methods... This will take a LOOOONG time and generate an insanely long console output :p\n");
					for(options.command=0;options.command<=4;options.command++)
					{
						for(options.dump_method=0;options.dump_method<=9;options.dump_method++)
						{
							fprintf (stderr, "Trying with command %d, method %d\n", options.command, options.dump_method);
							out = dologic(d, stats);
							if(out==true)
							{
								break;
							}
						}
						if(out==true)
						{
							fprintf (stderr, "Command %d and method %d combination worked!\n", options.command, options.dump_method);
							break;
						}
					}
				} else {
					out = dologic(d, stats);
				}	
				
				d = disc_destroy (d);
			}
		} else if (options.raw_in) {
			/* Convert raw image to ISO format */
			u = unscrambler_new ();
			
			if (options.gui)
				pfunc = (unscrambler_progress_func) progress_for_guis;
			else
				pfunc = (unscrambler_progress_func) progress;

			if ((out = unscrambler_unscramble_file (u, options.raw_in, options.iso_out, pfunc, &stats, &current_sector)))
				fprintf (stderr, "Unscrambling completed successfully!\n");
			else
				fprintf (stderr, "\nUnscrambling failed at sectors: %u..%u\n", current_sector, current_sector+15);

			u = unscrambler_destroy (u);
		} else {
			MY_ASSERT (0);
		}

		if (out) {
			duration = stats.end_time.tv_sec - stats.start_time.tv_sec;
			if (stats.end_time.tv_usec >= stats.start_time.tv_usec) {
				us = stats.end_time.tv_usec - stats.start_time.tv_usec;
			} else {
				if (duration > 0)
					duration--;
				us = USECS_PER_SEC + stats.end_time.tv_usec - stats.start_time.tv_usec;
			}
			duration += ((double) us / (double) USECS_PER_SEC);
			if (duration < 0)
				duration = 0;
			fprintf (stderr, "Operation took %.2f seconds\n", duration);

			ret = EXIT_SUCCESS;
		} else {
			ret = EXIT_FAILURE;
		}
		
		my_free (options.device);
		my_free (options.iso_out);
		my_free (options.raw_out);
		my_free (options.raw_in);
	}

	return (out);
}
