# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found.  Any number of sources could be listed here.
add_library (
	multihashlib
	${libmultihash_type}
	#SHARED
	#STATIC

	crc32.c
	edonkey.c
	md4.c
	md5.c
	multihash.c
	sha1.c
	crc32.h
	edonkey.h
	md4.h
	md5.h
	multihash.h
	sha1.h
)

set_target_properties (multihashlib PROPERTIES OUTPUT_NAME "multihash")

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
set_target_properties (multihashlib PROPERTIES SOVERSION 1.0.0)


#get_target_property(libhash_type multihashlib TYPE)
if (WIN32)
	if (libhash_type STREQUAL "SHARED")
#		MESSAGE ("Building libmultihash DLL")
		ADD_DEFINITIONS (-DMULTIHASH_BUILD_DLL)
	
		set_target_properties (multihashlib PROPERTIES DEFINE_SYMBOL MULTIHASH_EXPORTS)
	
		install (
			TARGETS multihashlib
			RUNTIME DESTINATION /
			#ARCHIVE DESTINATION lib
		)
	endif (libhash_type STREQUAL "SHARED")
else (WIN32)
	# Install stuff, only if a shared library is being built
	if (libhash_type STREQUAL "SHARED")
		install (
			TARGETS multihashlib
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib/static
		)
	endif (libhash_type STREQUAL "SHARED")
endif (WIN32)

