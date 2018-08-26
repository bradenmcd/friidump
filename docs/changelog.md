# Changelog

### 

## 0.5.3 (15/03/2010)

### Jackal, gorelord4e, themabus

- Fixed failing after 1st DL media layer with non-Hitachi methods.
- Fixed still hashing with 'nohash' parameter when resuming.
- Fixed resuming larger files (~4 GB).
- Fixed unscrambling larger files.
- Faster file unscrambling.
- Slight modifications to methods;
  possible performance increase with Hitachi based devices.
- Restructured methods and added some new ones.
- Added layer break information.
- Added current position output when error occurs.
- Added SH-D162A, SH-D162B, SH-D162C & SH-D162D as supported.
### 

# 0.5.2 (10/01/2010)

### Jackal, themabus

- Corrected handling of standard DVDs
  (type should be forced to 3, when dumping or unscrambling).
- Better response to 'speed' parameter.
- Uniform raw output for all devices: unscrambled data + headers.
- Slight performance increase (~1650 MB/h on LH-18A1H).
- Added LH-18A1P, LH-20A1H, LH-20A1P to list of supported devices.
### 

# 0.5.1 (01/12/2009)

### Jackal, themabus

- New command 'vanilla 2384'.

- Restructured methods, some now support optional parameters.

- Ability to select standard DVDs as source.

- Limited recognized Lite-On drives to LH-18A1H.

  ### 
# 0.5.0 (27/11/2009)

### Jackal, Truman, themabus

- Regions: Italy, France, Germany, Spain, Australia, PAL-X, PAL-Y.
- Updated publisher list from http://wiitdb.com/Company/HomePage
- Included GDR8082N & GDR8161B as supported Hitachi drives.
- Lite-On, Renesas & vanilla memory buffer access commands.
  Lite-On tested on LH-18A1H, should work on many more
  (LH*, SH, DH*, DW* & possibly other MediaTek drives)
- Shifted methods 1..4 to 0..3 and added new ones 4..6
  Associated known drives with default methods.
- Additional commandline parameters:
  stop, speed, command, type, size
- Some minor changes and fixes.
### 

# 0.4 (08/03/2008)

### mado3689

- Support for DL Wii DVDs.

  ### 

# 0.3 (06/10/2007)

- First public release.