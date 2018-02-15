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

AX_PTHREAD

AC_CHECK_DECL([pthread_barrier_wait],
              [AC_DEFINE([HAS_PTHREAD_BARRIER], [1],
                         [Define to 1 if pthread_barrier_wait is
                          available.])],
              [AC_DEFINE([HAS_PTHREAD_BARRIER], [0],
                         [Define to 1 if pthread_barrier_wait is
                          available.])],
              [[#include <pthread.h>]])

# TODO: use pkgconfig to detect papi

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([googletest])
AC_OUTPUT
