AC_INIT([HashJoin],[0.1.0])

AC_CONFIG_SRCDIR([README.mkdn])
AC_CONFIG_AUX_DIR([build-aux])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CC_C99

AX_PTHREAD

AC_ARG_WITH([papi], AS_HELP_STRING([--with-papi],
  [Enable papi hardware counters in benchmark]))

AS_IF([test "x$with_papi" = "xyes"],
      [PKG_CHECK_MODULES([PAPI],
        [papi >= 5],
        [
          AC_DEFINE([HAVE_PAPI], [1], [Define if papi is found])
        ]
      )])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([googletest])
AC_OUTPUT
