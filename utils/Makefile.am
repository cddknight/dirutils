AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = ndir addComment addcr casedir casefile hexDump inQuote lines listFunc memTest pfile rmcr procMonitor
AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64 $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
ndir_SOURCES = src/ndir.c
addComment_SOURCES = src/addComment.c
addComment_LDADD = -lreadline
addcr_SOURCES = src/addcr.c
casedir_SOURCES = src/casedir.c
casefile_SOURCES = src/casefile.c
hexDump_SOURCES = src/hexDump.c
lines_SOURCES = src/lines.c
inQuote_SOURCES = src/inQuote.c
listFunc_SOURCES = src/listFunc.c
memTest_SOURCES = src/memTest.c
pfile_SOURCES = src/pfile.c
procMonitor_SOURCES = src/procMonitor.c
rmcr_SOURCES = src/rmcr.c
EXTRA_DIST = ndirrc
Configdir = /etc
Config_DATA = ndirrc 

