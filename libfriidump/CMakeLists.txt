# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found.  Any number of sources could be listed here.
add_library (
	friidumplib
	${libfriidump_type}
	#SHARED
	#STATIC
	
 	brickblocker.h
	brickblocker.c
	byteorder.h
 	constants.h
 	disc.h
	disc.c
	dumper.h
	dumper.c
	dvd_drive.h
	dvd_drive.c
	hitachi.c
	ecma-267.h
	ecma-267.c
	lite-on.c
	misc.h
	misc.c
	renesas.c
	rs.h
	rs.c
	unscrambler.h
 	unscrambler.c
	vanilla_2064.c
	vanilla_2384.c
	win32compat.h
	win32compat.c
)

set_target_properties (friidumplib PROPERTIES OUTPUT_NAME "friidump")

include_directories (
	${FriiDump_SOURCE_DIR}/libmultihash
)

# Make sure the linker can find the Hello library once it is built.
link_directories (
	${FriiDump_BINARY_DIR}/libmultihash
)

# Link the executable to the Hello library.
target_link_libraries (
	friidumplib
	
	multihashlib
)

# Before making a release, the LTVERSION string should be modified.
# The string is of the form CURRENT:REVISION:AGE.
#
# CURRENT (C)
# The most recent interface number that this library implements.
#
# REVISION (R)
# The implementation number that this library implements.
#
# AGE (A)
# The difference between the newest and oldest interfaces that this
# library implements. In other works, the library implements all the
# interface numbers in the range from number 'CURRENT - AGE' to
# 'CURRENT'.
#
# This means that:
#
#   - If interfaces have been changed or added, but binary compatibility has
#     been preserved, change to C+1:0:A+1
#
#   - If binary compatibility has been broken (eg removed or changed
#     interfaces) change to C+1:0:0
#
#   - If the interface is the same as the previous version, change to C:R+1:A
#
#set_target_properties (friidumplib PROPERTIES SOVERSION 1.0.0)


# Windows stuff to correctly build DLL or static library
#get_target_property (libfriidump_type friidumplib TYPE)
if (WIN32)
	if (libfriidump_type STREQUAL "SHARED")
# 		MESSAGE ("Building libfriidump DLL")
		ADD_DEFINITIONS (-DFRIIDUMPLIB_BUILD_DLL)
		set_target_properties (friidumplib PROPERTIES DEFINE_SYMBOL FRIIDUMPLIB_EXPORTS)

		install (
			TARGETS friidumplib
			RUNTIME DESTINATION /
			#ARCHIVE DESTINATION lib
		)
	endif (libfriidump_type STREQUAL "SHARED")
else (WIN32)
	# Install stuff, only if a shared library is being built
	if (libfriidump_type STREQUAL "SHARED")
		install (
			TARGETS friidumplib
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib/static
		)
	endif (libfriidump_type STREQUAL "SHARED")
endif (WIN32)
