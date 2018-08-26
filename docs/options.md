# Help

FriiDump 0.5.3 - Copyright (C) 2007 Arep
This software comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under certain conditions; see COPYING for details.

Official support forum: http://wii.console-tribe.com

Forum for this UNOFFICIAL VERSION: http://forum.redump.org


Available command line options:

`-h, --help`	Show help
`-a, --autodump`Dump the disc to an ISO file with an automatically-generated name, resuming the dump if possible
`-g, --gui`Use more verbose output that can be easily parsed by a GUI frontend
`-d, --device <device>`Dump disc from device `<device>`
`-p, --stop`	Instruct device to stop disc rotation
`-c, --command <nr>`	Force memory dump command:

- 0 - vanilla 2064
- 1 - vanilla 2384
- 2 - Hitachi
- 3 - Lite-On
- 4 - Renesas

` -x, --speed <x>`Set streaming speed (1, 24, 32, 64, etc., where 1 = 150 KiB/s and so on)
` -T, --type <nr>`Force disc type:

- 				0 - GameCube
- 				1 - Wii
- 				2 - Wii_DL
- 				3 - DVD

` -S, --size <sectors>`Force disc size
` -r, --raw <file>`Output to file `<file>` in raw format (2064-byteectors)
` -i, --iso <file>`Output to file `<file>` in ISO format (2048-byte sectors)
` -u, --unscramble <file>`Convert (unscramble) raw image contained in`<file>` to ISO format
` -H, --nohash`Do not compute CRC32/MD5/SHA-1 hashes for generated files
` -s, --resume`Resume partial dump

### General

`--method0[=<req>,<exp>]`Use dumping method 0 (Optional argument specifies how many sectors to request from disc and read from cache at a time. Values should be separated with a comma. Default 16,16)

### Non-Streaming

` --method1[=<req>,<exp>]`Use dumping method 1 (Default 16,16)

`--method2[=<req>,<exp>]`Use dumping method 2 (Default 16,16)
`--method3[=<req>,<exp>]`Use dumping method 3 (Default 16,16)

### Streaming

 `--method4[=<req>,<exp>]`Use dumping method 4 (Default 27,27)
 `--method5[=<req>,<exp>]`Use dumping method 5 (Default 27,27)
` --method6[=<req>,<exp>]`Use dumping method 6 (Default 27,27)

### Hitachi

`--method7`Use dumping method 7 (Read and dump 5 blocks at a time, using streaming read)

`--method8`Use dumping method 8 (Read and dump 5 blocks at a time, using streaming read, using DMA)
`--method9`Use dumping method 9 (Read and dump 5 blocks at a time, using streaming read, using DMA and some speed tricks)