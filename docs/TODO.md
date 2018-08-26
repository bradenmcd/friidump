- Hashes are wrongly calculated on big-endian machines. This just needs fixing
  of some #define's in the multihash library.
- Add MacOS X-specific code. All that is needed is a way to send MMC commands
  to the DVD-ROM drive. Inspiration can be taken from the libcdio sources. If
  only I had a Mac... Same goes for *BSD, anyway.
- Maybe write a graphical client. I will not do this as I am no GUI fanatic,
  but I am sure many Windows users will still use RawDump as it has a GUI,
  while we do not. This is actually quite easy, as the program is split into a
  library (libfriidump) and a frontend. If anybody does it, please try to make
  a portable GUI, maybe by using GTK+, qt, wxWidgets or whatever similar
  toolkit you might find.
- Make FriiDump as fast as RawDump (i.e.: 2.5 hours for a Wii disc).
- Improve and better document the library API.
