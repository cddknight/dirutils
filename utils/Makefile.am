AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = ldir addComment casefile fileChop findFunny hexDump inQuote jsonParse lineDraw lineNum lines \
		listFunc mkTable modcr pfile rmCppCmt tabSpace xmlParse xmlCsv fixDir
AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64 $(S_DEPS_CFLAGS)
LIBS = $(S_DEPS_LIBS) -lm
ldir_SOURCES = src/ldir.c buildDate.h
addComment_SOURCES = src/addComment.c
addComment_LDADD = -lreadline
casefile_SOURCES = src/casefile.c
fileChop_SOURCES = src/fileChop.c
findFunny_SOURCES = src/findFunny.c
hexDump_SOURCES = src/hexDump.c
jsonParse_SOURCES = src/jsonParse.c
jsonParse_CFLAGS = $(P_DEPS_CFLAGS)
jsonParse_LDADD = $(P_DEPS_LIBS)
lineDraw_SOURCES = src/lineDraw.c
lineNum_SOURCES = src/lineNum.c
lines_SOURCES = src/lines.c
inQuote_SOURCES = src/inQuote.c
listFunc_SOURCES = src/listFunc.c
mkTable_SOURCES = src/mkTable.c
modcr_SOURCES = src/modcr.c
pfile_SOURCES = src/pfile.c
rmCppCmt_SOURCES = src/rmCppCmt.c
tabSpace_SOURCES = src/tabSpace.c
fixDir_SOURCES = src/fixDir.c
xmlParse_SOURCES = src/xmlParse.c
xmlParse_CFLAGS = $(P_DEPS_CFLAGS)
xmlParse_LDADD = $(P_DEPS_LIBS)
xmlCsv_SOURCES = src/xmlCsv.c
xmlCsv_CFLAGS = $(P_DEPS_CFLAGS)
xmlCsv_LDADD = $(P_DEPS_LIBS)
EXTRA_DIST = ldirrc
Configdir = /etc
Config_DATA = ldirrc 
BUILT_SOURCES = buildDate.h
CLEANFILES = buildDate.h
buildDate.h:
	setBuildDate -c
