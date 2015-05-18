#  Copyright (c) 2015, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

AC_PREREQ([2.62])
AC_INIT([geopm],m4_esyscmd([tr -d '\n' < VERSION]),[christopher.m.cantalupo@intel.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects tar-pax])
AC_CONFIG_HEADERS([config.h])

geopm_abi_version=0:0:0
AC_SUBST(geopm_abi_version)

LT_PREREQ([2.2.6])

AC_ARG_WITH([hwloc], [AS_HELP_STRING([--with-hwloc=PATH],
            [specify prefix directory for installed hwloc package.])])

if test "x$with_hwloc" != x; then
  CPPFLAGS="$CPPFLAGS -I$with_hwloc/include"
  LD_LIBRARY_PATH="$with_hwloc/lib:$LD_LIBRARY_PATH"
  LDFLAGS="$LDFLAGS -L$with_hwloc/lib"
fi
AC_ARG_WITH([hwloc-include], [AS_HELP_STRING([--with-hwloc-include=PATH],
            [specify directory for the installed hwloc include files.])])
if test "x$with_hwloc_include" != x; then
  CPPFLAGS="$CPPFLAGS -I$with_hwloc_include"
fi
AC_ARG_WITH([hwloc-lib], [AS_HELP_STRING([--with-hwloc-lib=PATH],
            [specify directory for installed hwloc library])])
if test "x$with_hwloc_lib" != x; then
  LD_LIBRARY_PATH="$with_hwloc_lib:$LD_LIBRARY_PATH"
  LDFLAGS="$LDFLAGS -L$with_hwloc_lib"
fi

# Checks for programs.
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AC_PROG_LIBTOOL
AM_PROG_CC_C_O


AC_CHECK_LIB([pthread], [pthread_mutex_init], [], [
    echo "pthread is required for this library"
    exit -1])
AC_CHECK_HEADER([pthread.h], [], [
    echo "pthread.h is required for this library"
    exit -1])
AC_CHECK_LIB([mpi], [MPI_Init], [], [
    echo "mpi is required for this library"
    exit -1])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h malloc.h netdb.h stddef.h stdint.h stdlib.h string.h sys/socket.h unistd.h])

AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime dup2 floor getcwd memset mkfifo pow sqrt strdup strstr])

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

AC_ARG_ENABLE([coverage],
  [AS_HELP_STRING([--enable-coverage], [Build with test coverage support and enable debug])],
[if test "x$enable_coverage" = "xno" ; then
  enable_coverage="0"
else
  enable_coverage="1"
  enable_debug="1"
fi
],
[enable_coverage="0"]
)

if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([GEOPM_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -O0 -g"
  CXXFLAGS="$CXXFLAGS -O0 -g"
fi
AC_SUBST([enable_debug])


if test "x$enable_coverage" = "x1" ; then
  AC_DEFINE([GEOPM_COVERAGE], [ ], [Enables test coverage reporting])
  CFLAGS="$CFLAGS --coverage"
  CXXFLAGS="$CXXFLAGS --coverage"
  LDFLAGS="$LDFLAGS --coverage -lgcov"
fi
AC_SUBST([enable_coverage])

CFLAGS="${CFLAGS} -Wall -Werror -std=c99"
CXXFLAGS="${CXXFLAGS} -Wall -Werror -std=c++11"
LDFLAGS="${LDFLAGS} -lrt -lhwloc"

AC_OUTPUT(Makefile)

# ============================================================================
# Print out the results of configuration.
AC_MSG_RESULT([===============================================================================])
AC_MSG_RESULT([ version    : ${VERSION}])
AC_MSG_RESULT([])
AC_MSG_RESULT([CC                 : ${CC}])
AC_MSG_RESULT([CPPFLAGS           : ${CPPFLAGS}])
AC_MSG_RESULT([CFLAGS             : ${CFLAGS}])
AC_MSG_RESULT([CXXFLAGS           : ${CXXFLAGS}])
AC_MSG_RESULT([LDFLAGS            : ${LDFLAGS}])
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
AC_MSG_RESULT([])
AC_MSG_RESULT([debug              : ${enable_debug}])
AC_MSG_RESULT([coverage           : ${enable_coverage}])
AC_MSG_RESULT([===============================================================================])
