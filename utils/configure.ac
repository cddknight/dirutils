AC_INIT([dircmdutils], [4.0.12])
AC_CONFIG_SRCDIR([src/ndir.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
REVISION=1
PKG_CHECK_MODULES(DEPS, dircmd)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)
AC_SUBST([REVISION])
AC_CONFIG_FILES([Makefile dircmdutils.spec])
AC_CONFIG_FILES([buildrpm.sh],[chmod +x buildrpm.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_CONFIG_FILES([builddeb.sh],[chmod +x builddeb.sh])
AC_OUTPUT

