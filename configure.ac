#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT
AC_CONFIG_SRCDIR([src/xmms-timidity.h])
AM_INIT_AUTOMAKE([xmms-timidity], [0.1])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

# Checks for libraries.
AM_PATH_XMMS([1.2.7], , AC_MSG_ERROR([*** XMMS >= 1.2.7 not installed - please install first ***]))
PKG_CHECK_MODULES([LIBTIMIDITY], [libtimidity], , AC_MSG_ERROR([*** libTiMidity not installed - please install first ***]))

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp strrchr])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 xmms-timidity.spec])
AC_OUTPUT
