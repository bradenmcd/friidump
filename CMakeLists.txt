cmake_minimum_required(VERSION 2.8)

# The name of our project is "HELLO".  CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
project (FriiDump)

if (MSVC)
    # msvc2005 deprecated warnings
    add_definitions (-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
else (MSVC)
    if (NOT WIN32)
      add_definitions (-fPIC)
    endif (NOT WIN32)
endif (MSVC)


include (TestBigEndian)

test_big_endian (CMAKE_WORDS_BIGENDIAN)


include (CheckIncludeFiles)

check_include_files (stdbool.h HAVE_STDBOOL_H)


include (CheckFunctionExists)

check_function_exists (fseeko HAVE_FSEEKO)
check_function_exists (ftello HAVE_FTELLO)
check_function_exists (fseek64 HAVE_FSEEK64)
check_function_exists (ftell64 HAVE_FTELL64)


include(CheckTypeSize)

set (CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64)

set (CMAKE_EXTRA_INCLUDE_FILES sys/types.h)
check_type_size ("off_t" OFF_T)
set (CMAKE_EXTRA_INCLUDE_FILES)

set (CMAKE_EXTRA_INCLUDE_FILES stdio.h)
check_type_size ("fpos_t" FPOS_T)
set (CMAKE_EXTRA_INCLUDE_FILES)

set (CMAKE_REQUIRED_DEFINITIONS)


option (
	DEBUG
	"Enable debugging messages"
	OFF
)


option (
	BUILD_STATIC_BINARY
	"Build a static binary (has precedence over ALL_LIBS_SHARED)"
	OFF
)

option (
	BUILD_ALL_LIBS_SHARED
	"Build all libraries as shared"
	OFF
)

if (BUILD_STATIC_BINARY)
	set (libmultihash_type STATIC)
	set (libfriidump_type STATIC)
elseif (BUILD_ALL_LIBS_SHARED)
	set (libmultihash_type SHARED)
	set (libfriidump_type SHARED)
else (BUILD_STATIC_BINARY)
	# This is how we build libraries by default
	set (libmultihash_type STATIC)
	set (libfriidump_type SHARED)
endif (BUILD_STATIC_BINARY)


# set (CMAKE_BUILD_TYPE superoptimized)
set (CMAKE_C_FLAGS_SUPEROPTIMIZED "-march=athlon-xp -m3dnow -O3 -funroll-all-loops")

set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")

# set (CMAKE_BUILD_TYPE release)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)
include_directories (
	${FriiDump_BINARY_DIR}
)

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
add_subdirectory (libmultihash)
add_subdirectory (libfriidump)
add_subdirectory (src)


if (WIN32)
	install (FILES AUTHORS DESTINATION / RENAME Authors.txt)
	#install (CODE "exec_program (${CMAKE_CURRENT_SOURCE_DIR}/utils/unix2dos.exe ${CMAKE_OUTPUT_BINARY_DIR} ARGS Authors.txt)")
	install (FILES ChangeLog DESTINATION / RENAME ChangeLog.txt)
	install (FILES COPYING DESTINATION / RENAME Copying.txt)
	install (FILES README DESTINATION / RENAME ReadMe.txt)
	install (FILES TODO DESTINATION / RENAME ToDo.txt)
endif (WIN32)


# CPack stuff
include (InstallRequiredSystemLibraries)

set (CPACK_PACKAGE_NAME "friidump")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dump Nintendo GameCube/Wii discs")
set (CPACK_PACKAGE_VENDOR "Arep")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "3")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_PACKAGE_INSTALL_DIRECTORY "FriiDump ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set (CPACK_PACKAGE_EXECUTABLES "friidump" "FriiDump")

set (CPACK_SOURCE_GENERATOR "TBZ2;ZIP")
set (CPACK_SOURCE_IGNORE_FILES
	"/CVS/"
	"/\\\\.svn/"
	"~$"
	"tags"
	"\\\\.kdevses$"
	"\\\\.kdevelop\\\\.pcs$"
	"/BUILD.*/"
	"/RELEASES/"
	"/utils/"
	"/doc.*/"
)
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

if(WIN32 AND NOT UNIX)
	set (CPACK_GENERATOR "NSIS;ZIP")
	# There is a bug in NSI that does not handle full unix paths properly. Make
	# sure there is at least one set of four (4) backlasshes.
#	set (CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
#	set (CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
	set (CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} installer")
	set (CPACK_NSIS_HELP_LINK "http:\\\\\\\\wii.console-tribe.com")
	set (CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
	set (CPACK_NSIS_CONTACT "arep@no.net")
	set (CPACK_NSIS_MODIFY_PATH ON)

	set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-Win32")
else(WIN32 AND NOT UNIX)
	set (CPACK_GENERATOR "TBZ2")
	set (CPACK_STRIP_FILES "bin/friidump;lib/libfriidump.so.1.0.0")
#   set (CPACK_SOURCE_STRIP_FILES "")
	set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-LinuxBin")
endif(WIN32 AND NOT UNIX)

include (CPack)
