# Make sure the compiler can find include files from our Hello library.
include_directories (
	${FriiDump_SOURCE_DIR}/libfriidump
)

# Make sure the linker can find the Hello library once it is built.
link_directories (
	${FriiDump_BINARY_DIR}/libfriidump
)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx".  The extensions are automatically found.
if (WIN32)
	set(
		additional_sources
	
		getopt-win32.h
		getopt-win32.c
		getopt_long-win32.c
	)
endif (WIN32)

add_executable (
	friidump

	friidump.c
	${additional_sources}
)

# Link the executable to the Hello library.
target_link_libraries (
	friidump
	
	friidumplib
)

if (WIN32)
	# We only want a version number on Windows (Is it any use?)
	set_target_properties (friidump	PROPERTIES VERSION 0.2)
	
	install(
		TARGETS friidump
		RUNTIME DESTINATION /
	)
else (WIN32)
	install(
		TARGETS friidump
		RUNTIME DESTINATION bin
	)
endif (WIN32)