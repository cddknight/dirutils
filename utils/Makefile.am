AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = ndir addComment addcr casedir casefile hexDump inQuote lines listFunc memTest pfile rmcr procMonitor
AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64 $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
ndir_SOURCES = ndir.c
addComment_SOURCES = addComment.c
addComment_LDADD = -lreadline 
addcr_SOURCES = addcr.c
casedir_SOURCES = casedir.c
casefile_SOURCES = casefile.c
hexDump_SOURCES = hexDump.c
lines_SOURCES = lines.c
inQuote_SOURCES = inQuote.c
listFunc_SOURCES = listFunc.c
memTest_SOURCES = memTest.c
pfile_SOURCES = pfile.c
procMonitor_SOURCES = procMonitor.c
rmcr_SOURCES = rmcr.c
EXTRA_DIST = ndirrc
Configdir = /etc
Config_DATA = ndirrc 

