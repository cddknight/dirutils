AC_INIT([libdircmd], [4.0.12])
AC_CONFIG_SRCDIR([src/dircmd.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
REVISION=4
AC_CHECK_HEADERS([selinux/selinux.h])
AC_CHECK_LIB(selinux, lgetfilecon, [DEPS_LIBS="$DEPS_LIBS -lselinux"]) 
AC_SUBST(DEPS_LIBS)
AC_SUBST([REVISION])
AC_CONFIG_FILES([Makefile libdircmd.spec pkgconfig/dircmd.pc])
AC_CONFIG_FILES([buildrpm.sh],[chmod +x buildrpm.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_CONFIG_FILES([builddeb.sh],[chmod +x builddeb.sh])
AC_OUTPUT

