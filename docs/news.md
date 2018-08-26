# News

Since the last official version, method 1 has been renamed to method 0 and it has undergone certain changes. Methods 2, 3, and 4 have been renamed to 7, 8 and 9 respectively. Method 0 should work with all drives as long as they are supported by one of the memory dump commands, so if a drive is unrecognized it is preferable to keep the method at 0, and try all the commands. If one (or more) combination(s) turns out to work, you can proceed then testing other methods with this command. In case none of the commands work, you could try to determine the drive's Read Buffer command parameters with supplied `BruteForce3C.exe`.

Generally the program's overall behaviour regarding the command line hasn't changed and you should be able to use the same options as with official versions, though in case you were using an unrecognized drive, which would nevertheless work with the Hitachi command, you'll need to set command to `2` now (e.g. `--command 2`) and method to `7`, `8` or `9`.

Performance have increased since official release and should be now about the same as with RawDump.

## Regarding supported drives:

1. `Hitachi-LG GDR8161B`, `GDR8162B`, `GDR8163B`, `GDR8164B`, GDR8082N Those drives can read GC/Wii media without swapping. Expected performance is 1600-1900 MB/h for `4B`, `3B` and 2100-2600 MB/h for `2B`, `1B`. The custom memory dump command is used, which returns 2064 bytes of data. It was reported that they can't read other (e.g. PC) discs this way though; this needs confirmation.

2. `Lite-On LH-18A1H`, `DVDRW LH-18A1P`, `DVDRW LH-20A1H`, `DVDRW LH-20A1P`
    Reading performance for PC DVDs can go up to 5000 MB/h, which means FriiDump's core as well as new methods are capable to output data at least at this rate. Reading performance for a GameCube disk was about 1600-1700 MB/h so likely this slowdown is caused by drive logic itself. Though I only had one GameCube game to test with, so possibly better results can be achieved depending on media. Best results were obtained, when using method 5 with parameter 16,27 (`--method5=16,27`). This combination isn't set as default because it can cause noticable delays depending on medium quality and to make methods more general for use with other devices. Lite-On won't read GC/Wii disks at all without swapping. Lite-On returns 2384 bytes of data (2064 + ECC) by means of vendor specific READ BUFFER command. Tested with models `LH-18A1H`, `LH-18A1P` and `LH-20A1H`.

3. `Plextor PX-760A`

  The Plextor would return 2064 bytes of already unscrambled data with READ BUFFER command. It works well with ordinary DVDs but due to the lack of streamed reading support is practically useless for GC/Wii dumping because of very low performance. Works nevertheless and could be used for some experiments and testing. Results from PX-760A.

4. `Toshiba Samsung SH-D162A`, `SH-D162B`, `SH-D162C`, `SH-D162D`
    Returns 2384 data bytes per sector like Lite-On does. Appears to support streamed reading but performance with tested model (`SH-D162D`) was somewhat low and unstable even with ordinary DVDs. Looks promising, if only a well-working method could be determined. Latest drives added, definitely need more testing at this point.
