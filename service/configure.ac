#  Copyright (c) 2015 - 2021, Intel Corporation
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

# 2.62 required for OpenMP macro and others
AC_PREREQ([2.62])
AC_INIT([geopm-service],m4_esyscmd([tr -d '\n' < VERSION]),[christopher.m.cantalupo@intel.com])
m4_define([serial_tests], [m4_esyscmd([
    vv=$(automake --version | head -n1 | sed 's|[^0-9.]*\([0-9.]*\).*|\1|');
    major=$(echo $vv | sed 's|\([0-9]*\).*|\1|');
    minor=$(echo $vv | sed 's|[0-9]*\.\([0-9]*\).*|\1|');
    if [ "$major" -gt 0 ] && [ "$minor" -gt 11 ]; then
        echo serial-tests;
    fi;
])])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects tar-pax] foreign serial_tests)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

geopm_abi_version=0:0:0
AC_SUBST(geopm_abi_version)
AC_DEFINE_UNQUOTED([GEOPM_ABI_VERSION], ["$geopm_abi_version"], [GEOPM shared object verion])

LT_PREREQ([2.2.6])

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

AC_ARG_ENABLE([nvml],
  [AS_HELP_STRING([--enable-nvml], [Enables use of the NVML library to support NVML board accelerators])],
[if test "x$enable_nvml" = "xno" ; then
  enable_nvml="0"
else
  enable_nvml="1"
fi
],
[enable_nvml="0"]
)
AC_SUBST([enable_nvml])
AM_CONDITIONAL([ENABLE_NVML], [test "x$enable_nvml" = "x1"])

AC_ARG_ENABLE([cnl-iogroup],
  [AS_HELP_STRING([--enable-cnl-iogroup], [Enable the CNL IOGroup])],
[if test "x$enable_cnl_iogroup" = "xno" ; then
  enable_cnl_iogroup="0"
else
  enable_cnl_iogroup="1"
fi
],
[enable_cnl_iogroup="0"]
)

if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([GEOPM_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -O0 -g"
  CXXFLAGS="$CXXFLAGS -O0 -g"
fi
AC_SUBST([enable_debug])

if test "x$enable_nvml" = "x1" ; then
  ENABLE_NVML=True
  AC_DEFINE([GEOPM_ENABLE_NVML], [ ], [Enables use of the NVML library to support NVML board accelerators])
else
  ENABLE_NVML=False
fi

if test "x$enable_cnl_iogroup" = "x1" ; then
  AC_DEFINE([GEOPM_CNL_IOGROUP], [ ], [Enables the CNL IOGroup])
fi
AC_SUBST([enable_cnl_iogroup])

AC_ARG_WITH([libnvml], [AS_HELP_STRING([--with-libnvml=PATH],
            [specify directory for installed libnvml package.])])
if test "x$with_libnvml" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_libnvml/include"
  LD_LIBRARY_PATH="$with_libnvml/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_libnvml/lib"
fi

AC_ARG_VAR([PYTHON],
           [PYTHON the Python interpreter used to build Python modules])

AC_ARG_WITH([python], [AS_HELP_STRING([--with-python=PATH],
            [specify the path to the python interpreter])])
if test "x$with_python" != x; then
  PYTHON=$with_python
else
  # On success, sets PYTHON to a version >= the specified version
  AM_PATH_PYTHON([3.6], [],
                 AC_MSG_ERROR([Failed to find a python >= 3.6. Use --with-python to explicitly specify an interpreter]))
fi

AC_ARG_VAR([GEOPM_CONFIG_PATH],
           [GEOPM_CONFIG_PATH The prefix to the path where GEOPM config files are stored. Default: /etc/geopm])
GEOPM_CONFIG_PATH=${GEOPM_CONFIG_PATH:=/etc/geopm}
AC_DEFINE_UNQUOTED([GEOPM_CONFIG_PATH],
          ["${GEOPM_CONFIG_PATH}"],
          [The prefix to the path where GEOPM config files are stored])

[EXTRA_CFLAGS="$EXTRA_CFLAGS -std=c99"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -std=c++11"]

[LDFLAGS="$EXTRA_LDFLAGS $LDFLAGS"]
[CFLAGS="$EXTRA_CFLAGS $CFLAGS"]
[CXXFLAGS="$EXTRA_CXXFLAGS $CXXFLAGS"]
[CPPFLAGS="$EXTRA_CPPFLAGS $CPPFLAGS"]

AC_SEARCH_LIBS([shm_open], [rt])
AC_SEARCH_LIBS([shm_unlink], [rt])
AC_SEARCH_LIBS([dlopen], [dl dld])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AM_PROG_CC_C_O
LT_INIT

AC_CHECK_LIB([pthread], [pthread_mutex_init], [], [
    echo "missing libpthread: POSIX thread interface is required"
    exit -1])
AC_CHECK_HEADER([pthread.h], [], [
    echo "missing pthread.h: POSIX thread interface is required"
    exit -1])

if test "x$enable_nvml" = "x1" ; then
    AC_SEARCH_LIBS([nvmlSystemGetCudaDriverVersion], [nvidia-ml])
    AC_CHECK_LIB([nvidia-ml], [nvmlSystemGetCudaDriverVersion], [], [
        echo "missing libnvidia-ml: NVML library is required, use --with-libnvml to specify location"
        exit -1])
    AC_CHECK_HEADER([nvml.h], [], [
        echo "missing nvml.h: NVML header is required, use --with-libnvml to specify location"
        exit -1])
fi

AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MMAP
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([clock_gettime floor ftruncate gethostname localtime_r munmap sqrt strerror])

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([that double is 64 bits])
AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([[#include <assert.h>]],
            [[static_assert(sizeof(double) == 8, "size of double is not 64 bits");]])
],[
    AC_MSG_RESULT(yes)
],[
    AC_MSG_RESULT(no)
    AC_MSG_FAILURE([sizeof double must equal 64 bits])
])

AC_LANG_POP([C++])

[AM_CFLAGS="$EXTRA_CFLAGS $AM_CFLAGS"]
[AM_CXXFLAGS="$EXTRA_CXXFLAGS $AM_CXXFLAGS"]
[AM_CPPFLAGS="$EXTRA_CPPFLAGS $AM_CPPFLAGS"]
[AM_FFLAGS="$EXTRA_FFLAGS $AM_FFLAGS"]
[AM_FCFLAGS="$EXTRA_FCFLAGS $AM_FCFLAGS"]
[AM_LDFLAGS="$EXTRA_LDFLAGS $AM_LDFLAGS"]

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_FFLAGS])
AC_SUBST([AM_FCFLAGS])
AC_SUBST([AM_LDFLAGS])

GEOPM_SOURCE_DIR=$(readlink -f $srcdir)
AC_DEFINE_UNQUOTED([GEOPM_SOURCE_DIR], ["$GEOPM_SOURCE_DIR"], [Root of the GEOPM source directory])

AC_CONFIG_FILES([Makefile geopm-service.spec geopmdpy/version.py])
AC_OUTPUT

# ============================================================================
# Print out the results of configuration.
AC_MSG_RESULT([===============================================================================])
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
AC_MSG_RESULT([PYTHON             : ${PYTHON}])
AC_MSG_RESULT([])
AC_MSG_RESULT([prefix             : ${prefix}])
AC_MSG_RESULT([exec_prefix        : ${exec_prefix}])
AC_MSG_RESULT([sbindir            : ${sbindir}])
AC_MSG_RESULT([includedir         : ${includedir}])
AC_MSG_RESULT([libdir             : ${libdir}])
AC_MSG_RESULT([datarootdir        : ${datarootdir}])
AC_MSG_RESULT([datadir            : ${datadir}])
AC_MSG_RESULT([mandir             : ${mandir}])
AC_MSG_RESULT([GEOPM_CONFIG_PATH  : ${GEOPM_CONFIG_PATH}])
AC_MSG_RESULT([])
AC_MSG_RESULT([debug              : ${enable_debug}])
AC_MSG_RESULT([nvml               : ${enable_nvml}])
AC_MSG_RESULT([===============================================================================])
