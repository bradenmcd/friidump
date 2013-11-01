mylibdir = $(top_builddir)/libfriidump
myincludedir = $(top_srcdir)/libfriidump

bin_PROGRAMS = friidump
friidump_SOURCES = config.h.cmake friidump.c
friidump_LDADD = $(mylibdir)/libfriidump.la
friidump_LDFLAGS = $(all_libraries)

# noinst_HEADERS =
INCLUDES = $(all_includes) -I$(myincludedir)

