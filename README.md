# FriiDump - A program to dump Nintendo Wii and GameCube discs

FriiDump is a program that lets you dump Nintendo Wii and GameCube discs from your computer, without using original Nintendo hardware. It basically performs the same functions as the famous "RawDump" program, but with a big difference, which should be clear straight from its name: FriiDump is free software, where "free" is to be intended both as in "free speech" and in "free beer". As such, FriiDump is distributed with its sources.

This leads to a number of good consequences:

- Having the sources available, it can be easily ported to different operating systems and hardware platforms. At the moment it is developed under a GNU/Linux system, but it also runs natively on Windows. A MacOS X version can be easily created, although I don't have a Mac, so I can't do it myself.
- Also, having the sources and these being well-organized (I know I'm a modest guy) allows support for new DVD-ROM drives to be added relatively easily. At the moment the same drives as RawDump are supported, but this might improve in the future, if anyone takes the effort... See README.technical for details.
- The sources might also be used as a reference for several things regarding Nintendo Wii/GameCube discs and the hacks used to read them on an ordinary drive.

Furthermore, FriiDump also features some functional improvements over RawDump:
- FriiDump can use 4 different methods to read the disc, with different performance.
- FriiDump dumps a lot of useful information about the discs it dumps, such as whether the disc contains an update or not, which can help avoid bricking your Wii ;).
- FriiDump calculates the CRC32, MD4, MD5, SHA-1 and ED2K hashes of dumped discs, so you can immediately know if your dump is good or not, by comparing the hashes with the well-known ones available on several Internet sites.
- FriiDump comes in the form of a library and a command-line front-end, which allows its functions to be easily reused in other programs.

Unfortunately, there is also a main downfall:
- Even the fastest dump method used by FriiDump is not as fast as RawDump (but not that much slower, either, see the table below).

Anyway, I'm sure that people who cannot use RawDump (i.e.: GNU/Linux, *BSD and MacOS X users) will be happy anyway. Besides, you get the sources, so you can improve them yourself.

Note that FriiDump is only useful to dump *original* Nintendo discs. To dump backup copies you can use any DVD-dumping program (i.e.: `dd` under UNIX ;)).

FriiDump came to existance thanks to the work by a lot of people, most of which are probably not aware of this fact ;). Please see the AUTHORS file for the credits.

## Supported drives
At the moment the same drives as RawDump are supported. This is due to various reasons, explained in the README.technical file, which also contains information about what is needed to add support for more drives.

Currently supported drives are:
- LG GDR-8161B (untested, but should work)
- LG GDR-8162B (ditto)
- LG GDR-8163B (HL-DT-ST/DVD-ROM GDR8163B)
- LG GDR-8164B (HL-DT-ST/DVD-ROM GDR8164B)

Other drives might work, most likely those based on the Hitachi MN103 microcontroller. If you find any of them, please report so that they can be added to the compatibility list.

## Installation
If you are a Windows user, probably you will have downloaded the binaries, either zipped or together with an installer, so the installation should be straightforward.

If you downloaded the sources, you will need to compile them. FriiDump uses CMake, for easy portability, so you will need to get it from cmake.org. On Windows you will also need a compiler like Visual Studio (the only tested one, so far) or CygWin/MinGW. On UNIX just do the following, from the directory where you unpacked the sources into:

```bash
mkdir BUILD
cd BUILD
cmake ..
make
make install
```

Linux-specific note: You need root privileges to issue certain commands to the DVD-ROM drive. Hence you have the following possibilities:
- Run FriiDump as root: discouraged.

- Run it through sudo: better but nevertheless discouraged.

- Set the setuid bit on the executable: this is the recommended way to run FriiDump under Linux. This way, the code run with superuser privileges will be reduced to a minimum, guaranteeing a certain level of security (note that security-related bugs might exist anyway!!!). Also note that, even when the setuid bit is set, the attempt to open the drive for reading will be done after privileges have been dropped, so you will need explicit read access to the DVD-ROM drive. Usually having the system administrator add you to the "cdrom" group is enough. To set the setuid bit on the executable, run as root:

```bash
  chown root:root /usr/local/bin/friidump
  chmod u+s /usr/local/bin/friidump
```

## Usage
FriiDump is a command-line program, so you will need to run it from a terminal or a command-prompt under Windows. The basic usage is as follows:

```bash
friidump -d <drive> -a
```

where `<drive>` will usually be something like `/dev/hda` on Unix-like systems, and something like `e:` for Windows users. With this command, the disc will be dumped to an ISO image file with an automatically-chosen name. Drop `-a` and use the `-i` option if you prefer to specify the filename yourself. If you want to resume an existing dump, use `-s`. If you want to dump the disc to a raw format image file, use `-r`. Note that you can create a raw and an ISO image at the same time.

Other options you might want to use are `-1` through `-4`, to set the dump method, although the default is method 4, which is the fastest one, so most likely you will not need them.

Finally, use `-h` for a listing of all available options.

## Performance
As stated above, FriiDump is not as fast as RawDump. On my PC (Athlon64 3200+), performance is as follows:

| Method | Dump speed  | Gamecube disc dump time | Wii disc dump time |
| ------ | ----------- | ----------------------- | ------------------ |
| 1      | Too slow ;) | Eternity                | More than eternity |
| 2      | ~570 MB/h   | 2.5 hours               | 8 hours            |
| 3      | ~740 MB /h  | 2 hours                 | 6 hours            |
| 4      | ~1250 MB/h  | 1.2 hours               | 3.5 hours          |



## Support
I'm releasing this program under the nickname of "Arep". This is because I am not sure about the legal status of the program, and I do not want to encounter any consequences. Actually, I'm pretty sure FriiDump goes against the DMCA, being a program that circumvents copy-protection, but it might be objected that the format used by Nintendo discs is not a copy-protection method, but just their own, undocumented, disc format. Although, I think it can be freely used in Europe and other coutries without laws similar to the DMCA.

For the same reason, I am not putting an e-mail address here (that @no.net you find in the program is obviously a pun), but support will be provided through the forums of the Italian ConsoleTribe forum, at http://wii.console-tribe.com. If you need help, just open a thread in any section there, even in English: I will *not* reply, but you might stand assured I will read everything you write. FriiDump users are encouraged to help each other there ;).

Patches are welcome, too: just attach them to your post, and maybe put something like "[PATCH]" in the topic subject, so that I can easily spot them.

New releases will be announced on that forum, and also on QJ.net, if I find a good way to notify them.

If you want to donate to the project, do not do it, and donate to one of the free Wii modchip projects out there, such as OpenWii, WiiFree or YAOSM.

## Disclaimer
FriiDump is distributed under the GNU General Public License version 2. See the COPYING file for details.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Also, please note that this program is not meant to be used to spread game piracy, but rather to be an instrument to make backups of your own precious legally-bought games.
