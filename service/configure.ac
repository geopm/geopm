#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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

geopm_abi_version=1:0:0
AC_SUBST(geopm_abi_version)
AC_DEFINE_UNQUOTED([GEOPM_ABI_VERSION], ["$geopm_abi_version"], [GEOPM shared object version])
AC_DEFINE([GEOPM_SERVICE_BUILD], [], [Building objects used by geopm-service])

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

AC_ARG_ENABLE([docs],
  [AS_HELP_STRING([--disable-docs], [Disable the use of the Python sphinx tool to generate documentation])],
[if test "x$enable_docs" = "xno" ; then
  enable_docs="0"
else
  enable_docs="1"
fi
],
[enable_docs="1"]
)

AC_SUBST([enable_docs])
AM_CONDITIONAL([ENABLE_DOCS], [test "x$enable_docs" = "x1"])

AC_ARG_ENABLE([systemd],
  [AS_HELP_STRING([--disable-systemd], [Disable the use of the sd_bus interface of libsystemd to support the ServiceIOGroup])],
[if test "x$enable_systemd" = "xno" ; then
  enable_systemd="0"
else
  enable_systemd="1"
fi
],
[enable_systemd="1"]
)

AC_SUBST([enable_systemd])
AM_CONDITIONAL([ENABLE_SYSTEMD], [test "x$enable_systemd" = "x1"])

if test "x$enable_systemd" = "x1" ; then
  ENABLE_SYSTEMD=True
  AC_DEFINE([GEOPM_ENABLE_SYSTEMD], [ ], [Enables the use of the sd_bus interface of libsystemd to support the ServiceIOGroup])
else
  ENABLE_SYSTEMD=False
fi

AC_ARG_ENABLE([levelzero],
  [AS_HELP_STRING([--enable-levelzero], [Enables use of the levelzero library to support levelzero board GPUs])],
[if test "x$enable_levelzero" = "xno" ; then
  enable_levelzero="0"
else
  enable_levelzero="1"
fi
],
[enable_levelzero="0"]
)
AC_SUBST([enable_levelzero])
AM_CONDITIONAL([ENABLE_LEVELZERO], [test "x$enable_levelzero" = "x1"])

AC_ARG_ENABLE([nvml],
  [AS_HELP_STRING([--enable-nvml], [Enables use of the NVML library to support NVML board GPUs])],
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

AC_ARG_ENABLE([dcgm],
  [AS_HELP_STRING([--enable-dcgm], [Enables use of the DCGM library to support NVIDIA board GPUs])],
[if test "x$enable_dcgm" = "xno" ; then
  enable_dcgm="0"
else
  enable_dcgm="1"
fi
],
[enable_dcgm="0"]
)
AC_SUBST([enable_dcgm])
AM_CONDITIONAL([ENABLE_DCGM], [test "x$enable_dcgm" = "x1"])

AC_ARG_ENABLE([cnl-iogroup],
  [AS_HELP_STRING([--enable-cnl-iogroup], [Enable the CNL IOGroup])],
[if test "x$enable_cnl_iogroup" = "xno" ; then
  enable_cnl_iogroup="0"
else
  enable_cnl_iogroup="1"
fi
],
[enable_cnl_iogroup="1"]
)

AC_ARG_ENABLE([io-uring],
  [AS_HELP_STRING([--disable-io-uring],
                  [Disable the use of libiouring for batch IO operations])],
[if test "x$enable_io_uring" = "xno" ; then
  enable_io_uring="0"
else
  enable_io_uring="1"
  AC_DEFINE([GEOPM_HAS_IO_URING], [ ], [Enables liburing in GEOPM])
fi
],
[enable_io_uring="1"]
)
AC_SUBST([enable_io_uring])
AM_CONDITIONAL([ENABLE_IO_URING], [test "x$enable_io_uring" = "x1"])

AC_PATH_PROG([PROTOC], [protoc], [])
AS_IF([test "x$PROTOC" = x], [AC_MSG_ERROR([protoc not found])])
PKG_CHECK_MODULES([PROTOBUF], [protobuf >= 3.0.0])
AC_SUBST([PROTOBUF_CFLAGS])
AC_SUBST([PROTOBUF_LIBS])
PKG_CHECK_MODULES([GRPC], [grpc++ >= 1.3.0 grpc >= 3.0.0])
AC_SUBST([GRPC_CFLAGS])
AC_SUBST([GRPC_LIBS])
AC_PATH_PROG([GRPC_CPP_PLUGIN], [grpc_cpp_plugin])
AS_IF([test "x$GRPC_CPP_PLUGIN" = x], [
  AC_MSG_ERROR([grpc_cpp_plugin not found])
])

if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([GEOPM_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -O0 -g"
  CXXFLAGS="$CXXFLAGS -O0 -g"
fi
AC_SUBST([enable_debug])

if test "x$enable_coverage" = "x1" ; then
  AC_DEFINE([GEOPM_COVERAGE], [ ], [Enables test coverage reporting])
  EXTRA_CFLAGS="$EXTRA_CFLAGS --coverage"
  EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS --coverage"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS --coverage -lgcov"
fi
AC_SUBST([enable_coverage])

if test "x$enable_dcgm" = "x1" ; then
  ENABLE_DCGM=True
  AC_DEFINE([GEOPM_ENABLE_DCGM], [ ], [Enables use of the DCGM library to support NVIDA board GPUs])
fi

if test "x$enable_nvml" = "x1" ; then
  ENABLE_NVML=True
  AC_DEFINE([GEOPM_ENABLE_NVML], [ ], [Enables use of the NVML library to support NVML board GPUs])
else
  ENABLE_NVML=False
    if test "x$enable_levelzero" = "x1" ; then
        ENABLE_LEVELZERO=True
        AC_DEFINE([GEOPM_ENABLE_LEVELZERO], [ ], [Enables use of the levelzero library to support levelzero board GPUs])
    else
        ENABLE_LEVELZERO=False
    fi
fi

if test "x$enable_cnl_iogroup" = "x1" ; then
  AC_DEFINE([GEOPM_CNL_IOGROUP], [ ], [Enables the CNL IOGroup])
fi
AC_SUBST([enable_cnl_iogroup])

AC_ARG_WITH([nvml], [AS_HELP_STRING([--with-nvml=PATH],
            [specify directory for installed nvml package.])])
if test "x$with_nvml" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_nvml/include"
  LD_LIBRARY_PATH="$with_nvml/lib64:$with_nvml/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_nvml/lib -L$with_nvml/lib64"
fi

AC_ARG_WITH([dcgm], [AS_HELP_STRING([--with-dcgm=PATH],
            [specify directory for installed dcgm package.])])
if test "x$with_dcgm" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_dcgm/include"
  LD_LIBRARY_PATH="$with_dcgm/lib64:$with_dcgm/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_dcgm/lib -L$with_dcgm/lib64"
fi

AC_ARG_WITH([levelzero], [AS_HELP_STRING([--with-levelzero=PATH],
            [specify directory for installed levelzero package.])])
if test "x$with_levelzero" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_levelzero/include"
  LD_LIBRARY_PATH="$with_levelzero/lib64:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_levelzero/lib64"
fi

AC_ARG_WITH([liburing], [AS_HELP_STRING([--with-liburing=PATH],
            [specify directory for the installed liburing package.])])
if test "x$with_liburing" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_liburing/include"
  LD_LIBRARY_PATH="$with_liburing/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_liburing/lib"
fi
AC_ARG_WITH([liburing-include], [AS_HELP_STRING([--with-liburing-include=PATH],
            [specify directory for the installed liburing include files.])])
if test "x$with_liburing_include" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_liburing_include"
fi
AC_ARG_WITH([liburing-lib], [AS_HELP_STRING([--with-liburing-lib=PATH],
            [specify directory for the installed liburing library.])])
if test "x$with_liburing_lib" != x; then
  LD_LIBRARY_PATH="$with_liburing_lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_liburing_lib"
fi

AC_CHECK_HEADER([nmmintrin.h], [AC_DEFINE([GEOPM_HAS_SSE42], [1], [nmmintrin.h is available])], [])

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

[EXTRA_CFLAGS="$EXTRA_CFLAGS -std=c99 -msse4.2"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -std=c++17 -msse4.2"]

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

if test "x$enable_systemd" = "x1" ; then
    AC_CHECK_LIB([systemd], [sd_bus_open_system], [], [
        echo "missing libsystemd or does libsystemd does not support sd_bus interface, to disable ServiceIOGroup configure with --disable-systemd"
        exit -1])
    AC_CHECK_HEADER([systemd/sd-bus.h], [], [
        echo "missing systemd/sd-bus.h: sd_bus interface is required"
        exit -1])
fi

if test "x$enable_dcgm" = "x1" ; then
    AC_SEARCH_LIBS([dcgmInit], [dcgm])
    AC_CHECK_LIB([dcgm], [dcgmInit], [], [
        echo "missing dcgm: DCGM library is required, use --with-dcgm to specify location"
        exit -1])
    AC_CHECK_HEADER([dcgm_structs.h], [], [
        echo "missing dcgm_structs.h: DCGM headers are required, use --with-dcgm to specify location"
        exit -1])
fi

if test "x$enable_nvml" = "x1" ; then
    AC_SEARCH_LIBS([nvmlSystemGetCudaDriverVersion], [nvidia-ml])
    AC_CHECK_LIB([nvidia-ml], [nvmlSystemGetCudaDriverVersion], [], [
        echo "missing libnvidia-ml: NVML library is required, use --with-nvml to specify install prefix for NVML path"
        exit -1])
    AC_CHECK_HEADER([nvml.h], [], [
        echo "missing nvml.h: NVML header is required, use --with-nvml to specify install prefix for NVML path"
        exit -1])
else
    AC_LANG_PUSH([C++])
    if test "x$enable_levelzero" = "x1" ; then
        AC_SEARCH_LIBS([zeInit], [ze_loader])
        AC_CHECK_LIB([ze_loader], [zeInit], [], [
            echo "missing libze_loader: LevelZero library is required, use --with-levelzero to specify location"
            exit -1])
        AC_CHECK_HEADER([level_zero/ze_api.h], [], [
            echo "missing ze_api.h: LevelZero header is required, use --with-levelzero to specify location"
            exit -1])
    fi
    AC_LANG_POP([C++])
fi

if test "x$enable_io_uring" = "x1" ; then
    AC_SEARCH_LIBS([io_uring_queue_init], [uring])
    AC_CHECK_LIB([uring], [io_uring_queue_init], [], [
        echo "missing liburing: Configure with --with-liburing or --with-liburing-lib to specify the location or --disable-io-uring to avoid this dependency"
        exit -1])
    AC_CHECK_HEADER([liburing.h], [], [
        echo "missing liburing.h: Configure with --with-liburing or --with-liburing-include to specify the location or --disable-io-uring to avoid this dependency"
        exit -1],
        [#define _DEFAULT_SOURCE
         #define _POSIX_C_SOURCE 200809L
         #include <liburing.h>])
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

m4_include([m4/ax_check_compile_flag.m4])
AX_CHECK_COMPILE_FLAG([-fp-model="fast"], [FASTMATH="-fp-model=fast"], [FASTMATH="-ffast-math"], [], [])
AX_CHECK_COMPILE_FLAG([-fp-model="precise"], [NO_FASTMATH="-fp-model=precise"], [NO_FASTMATH="-fno-fast-math"], [], [])

[EXTRA_CFLAGS="$EXTRA_CFLAGS -Wall -Werror $NO_FASTMATH $GRPC_CFLAGS $PROTOBUF_CFLAGS"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -Wall -Werror $NO_FASTMATH $GRPC_CFLAGS $PROTOBUF_CFLAGS"]
[LIBS="$LIBS $GRPC_LIBS $PROTOBUF_LIBS"]

[AM_CFLAGS="$EXTRA_CFLAGS $AM_CFLAGS"]
[AM_CXXFLAGS="$EXTRA_CXXFLAGS $AM_CXXFLAGS"]
[AM_CPPFLAGS="$EXTRA_CPPFLAGS $AM_CPPFLAGS"]
[AM_FFLAGS="$EXTRA_FFLAGS $AM_FFLAGS"]
[AM_FCFLAGS="$EXTRA_FCFLAGS $AM_FCFLAGS"]
[AM_LDFLAGS="$EXTRA_LDFLAGS $AM_LDFLAGS"]

AC_SUBST([AM_CFLAGS])
AC_SUBST([FASTMATH])
AC_SUBST([NO_FASTMATH])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_FFLAGS])
AC_SUBST([AM_FCFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_SUBST_FILE([JSON_SCHEMA_ACTIVE_SESSIONS])
JSON_SCHEMA_ACTIVE_SESSIONS=json_schemas/active_sessions.schema.json

GEOPM_SOURCE_DIR=$(readlink -f $srcdir)
AC_DEFINE_UNQUOTED([GEOPM_SOURCE_DIR], ["$GEOPM_SOURCE_DIR"], [Root of the GEOPM source directory])

AC_CONFIG_FILES([Makefile geopm-service.spec geopmdpy/version.py geopmdpy/schemas.py geopmdrs/Cargo.toml debian/changelog])
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
AC_MSG_RESULT([coverage           : ${enable_coverage}])
AC_MSG_RESULT([docs               : ${enable_docs}])
AC_MSG_RESULT([dcgm               : ${enable_dcgm}])
AC_MSG_RESULT([nvml               : ${enable_nvml}])
AC_MSG_RESULT([levelzero          : ${enable_levelzero}])
AC_MSG_RESULT([io_uring           : ${enable_io_uring}])
AC_MSG_RESULT([===============================================================================])
