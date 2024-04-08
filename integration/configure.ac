#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([geopm-integration],m4_esyscmd([tr -d '\n' < VERSION]),[brad.geltz@intel.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects tar-pax] foreign)
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Build debugging code and compile with -O0 -g])],
[if test "x$enable_debug" = "xno" ; then
  enable_debug="0"
else
  enable_debug="1"
fi
],
[enable_debug="0"]
)

if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([GEOPM_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -Og -g"
  CXXFLAGS="$CXXFLAGS -Og -g"
fi
AC_SUBST([enable_debug])


AC_CHECK_LIB([geopmd], [geopm_pio_read_signal], [], [
    echo "missing libgeopmd: Install the geopm-service and use --with-geopmd or --with-geopmd-lib to specify location"
    exit -1])

AC_CHECK_HEADER([geopm_pio.h], [], [
    echo "missing geopm_pio.h: Install the geopm-service and use --with-geopmd or --with-geopmd-include to specify location"
    exit -1])

AC_CHECK_LIB([geopm], [geopm_prof_region], [], [
    echo "missing libgeopm: Install the geopm-runtime and use --with-geopm or --with-geopm-lib to specify location"
    exit -1])

AC_CHECK_HEADER([geopm_prof.h], [], [
    echo "missing geopm_prof.h: Install the geopm-runtime and use --with-geopm or --with-geopm-include to specify location"
    exit -1])


# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

CC_VERSION=$(${CC} --version | head -n1)
# ============================================================================
# Print out the results of configuration.
AC_MSG_RESULT([===============================================================================])
AC_MSG_RESULT([CC version         : ${CC_VERSION}])
AC_MSG_RESULT([version            : ${VERSION}])
AC_MSG_RESULT([])
AC_MSG_RESULT([CPPFLAGS           : ${CPPFLAGS}])
AC_MSG_RESULT([AM_CPPFLAGS        : ${AM_CPPFLAGS}])
AC_MSG_RESULT([CC                 : ${CC}])
AC_MSG_RESULT([CFLAGS             : ${CFLAGS}])
AC_MSG_RESULT([AM_CFLAGS          : ${AM_CFLAGS}])
AC_MSG_RESULT([CXX                : ${CXX}])
AC_MSG_RESULT([CXXFLAGS           : ${CXXFLAGS}])
AC_MSG_RESULT([AM_CXXFLAGS        : ${AM_CXXFLAGS}])
AC_MSG_RESULT([LDFLAGS            : ${LDFLAGS}])
AC_MSG_RESULT([AM_LDFLAGS         : ${AM_LDFLAGS}])
AC_MSG_RESULT([LD_LIBRARY_PATH    : ${LD_LIBRARY_PATH}])
AC_MSG_RESULT([LIBS               : ${LIBS}])
AC_MSG_RESULT([])
AC_MSG_RESULT([prefix             : ${prefix}])
AC_MSG_RESULT([exec_prefix        : ${exec_prefix}])
AC_MSG_RESULT([sbindir            : ${sbindir}])
AC_MSG_RESULT([includedir         : ${includedir}])
AC_MSG_RESULT([libdir             : ${libdir}])
AC_MSG_RESULT([datarootdir        : ${datarootdir}])
AC_MSG_RESULT([datadir            : ${datadir}])
AC_MSG_RESULT([mandir             : ${mandir}])
AC_MSG_RESULT([bashcompdir        : ${bashcompdir}])
AC_MSG_RESULT([GEOPM_CONFIG_PATH  : ${GEOPM_CONFIG_PATH}])
AC_MSG_RESULT([])
AC_MSG_RESULT([debug              : ${enable_debug}])
AC_MSG_RESULT([===============================================================================])
