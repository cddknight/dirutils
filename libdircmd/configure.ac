AC_INIT([libdircmd], [5.0])
AC_CONFIG_SRCDIR([src/dircmd.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
REVISION=1
AC_CHECK_HEADERS([selinux/selinux.h sys/acl.h alues.h openssl/md5.h openssl/sha.h])
AC_CHECK_LIB(crypto, MD5_Init, [DEPS_LIBS="$DEPS_LIBS -lcrypto"])
AC_CHECK_LIB(selinux, lgetfilecon, [DEPS_LIBS="$DEPS_LIBS -lselinux"]) 
AC_CHECK_LIB(acl, acl_get_file, [DEPS_LIBS="$DEPS_LIBS -lacl"]) 
AC_SUBST(DEPS_LIBS)
AC_SUBST([REVISION])
AC_CONFIG_FILES([Makefile libdircmd.spec pkgconfig/dircmd.pc])
AC_CONFIG_FILES([buildrpm.sh],[chmod +x buildrpm.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_CONFIG_FILES([builddeb.sh],[chmod +x builddeb.sh])
AC_OUTPUT

