AC_INIT([dircmdutils],[5.3])
AC_CONFIG_SRCDIR([src/ldir.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_CC_C_O
LT_INIT
AC_PROG_INSTALL
REVISION=1
PKG_CHECK_MODULES([S_DEPS], [dircmd])
PKG_CHECK_MODULES([P_DEPS], [libxml-2.0 glib-2.0 json-glib-1.0])
AC_CHECK_HEADERS([sys/acl.h values.h])
AC_SUBST(S_DEPS_CFLAGS)
AC_SUBST(S_DEPS_LIBS)
AC_SUBST(P_DEPS_CFLAGS)
AC_SUBST(P_DEPS_LIBS)
AC_SUBST([REVISION])
AC_CONFIG_FILES([Makefile dircmdutils.spec])
AC_CONFIG_FILES([buildpkg.sh],[chmod +x buildpkg.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_OUTPUT

