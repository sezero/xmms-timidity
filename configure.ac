#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([xmms-timidity],[0.2])
AC_CONFIG_AUX_DIR([autotools])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_SRCDIR([src/xmms-timidity.h])
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

#LT_INIT([win32-dll disable-static])
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Checks for libraries.
AM_PATH_XMMS([1.2.7], , AC_MSG_ERROR([*** XMMS >= 1.2.7 not installed - please install first ***]))
PKG_CHECK_MODULES([LIBTIMIDITY], [libtimidity], , AC_MSG_ERROR([*** libTiMidity not installed - please install first ***]))

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp strrchr])

# The code depends on old glib: see if compiler supports -fgnu89-inline
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fgnu89-inline"
AC_CACHE_CHECK([if compiler supports -fgnu89-inline],[ac_cv_gnu89_inline],
	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_COMPILE([void foo(void) {}],
		[],
		[ac_cv_gnu89_inline=yes],
		[ac_cv_gnu89_inline=no])
	AC_LANG_RESTORE)
if test $ac_cv_gnu89_inline = no
then
   CFLAGS="$ac_save_CFLAGS"
fi

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 xmms-timidity.spec])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
