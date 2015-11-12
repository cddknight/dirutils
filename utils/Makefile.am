AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = ldir addComment addcr casedir casefile findFunny hexDump inQuote lines listFunc memTest pfile rmcr procMonitor xmlParse
AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64 $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
ldir_SOURCES = src/ldir.c
addComment_SOURCES = src/addComment.c
addComment_LDADD = -lreadline
addcr_SOURCES = src/addcr.c
casedir_SOURCES = src/casedir.c
casefile_SOURCES = src/casefile.c
findFunny_SOURCES = src/findFunny.c
hexDump_SOURCES = src/hexDump.c
lines_SOURCES = src/lines.c
inQuote_SOURCES = src/inQuote.c
listFunc_SOURCES = src/listFunc.c
memTest_SOURCES = src/memTest.c
pfile_SOURCES = src/pfile.c
procMonitor_SOURCES = src/procMonitor.c
rmcr_SOURCES = src/rmcr.c
xmlParse_SOURCES = src/xmlParse.c
xmlParse_LDADD = -lxml2
EXTRA_DIST = ldirrc
Configdir = /etc
Config_DATA = ldirrc 

