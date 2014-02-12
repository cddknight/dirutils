AUTOMAKE_OPTIONS = dist-bzip2
AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64
lib_LTLIBRARIES = libdircmd.la
libdircmd_la_SOURCES = dircmd.c display.c match.c list.c crc.c config.c dircmd.h
libdircmd_la_LDFLAGS = -version-info 4:9
libdircmd_la_LIBADD = $(DEPS_LIBS)
nobase_include_HEADERS = dircmd.h
pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = pkgconfig/dircmd.pc

