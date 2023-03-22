AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = casedir memTest procMonitor
AM_CPPFLAGS = $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
casedir_SOURCES = src/casedir.c buildDate.h
memTest_SOURCES = src/memTest.c
procMonitor_SOURCES = src/procMonitor.c
BUILT_SOURCES = buildDate.h
CLEANFILES = buildDate.h
buildDate.h:
	setBuildDate -c
