#  Copyright (c) 2015 - 2024, Intel Corporation
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

geopm_abi_version=2:0:0
AC_SUBST(geopm_abi_version)
AC_DEFINE_UNQUOTED([GEOPM_ABI_VERSION], ["$geopm_abi_version"], [GEOPM shared object version])
AC_DEFINE([GEOPM_SERVICE_BUILD], [], [Building objects used by geopm-service])

DATE=`LANG=C date -R`
AC_SUBST(DATE)
AC_DEFINE_UNQUOTED([DATE], ["$DATE"], [Date string suitable for debian changelog])

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

AC_ARG_ENABLE([asan],
  [AS_HELP_STRING([--enable-asan], [Build and test with address sanitizers enabled])],
[if test "x$enable_asan" = "xno" ; then
  enable_asan="0"
else
  enable_asan="1"
  enable_debug="1"
fi
],
[enable_asan="0"]
)

AC_ARG_ENABLE([fuzz],
  [AS_HELP_STRING([--enable-fuzz], [Build fuzz tests])],
[if test "x$enable_fuzz" = "xno" ; then
  enable_fuzz="0"
else
  enable_fuzz="1"
  enable_asan="1"
  enable_debug="1"
fi
],
[enable_fuzz="0"]
)
AC_SUBST([enable_fuzz])
AM_CONDITIONAL([ENABLE_FUZZTESTS], [test "x$enable_fuzz" = "x1"])

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

AC_ARG_ENABLE([libcap],
  [AS_HELP_STRING([--disable-libcap], [Disable the use of libcap and use syscall instead])],
[if test "x$enable_libcap" = "xno" ; then
  enable_libcap="0"
else
  enable_libcap="1"
fi
],
[enable_libcap="1"]
)

AC_SUBST([enable_libcap])
AM_CONDITIONAL([ENABLE_LIBCAP], [test "x$enable_libcap" = "x1"])

if test "x$enable_libcap" = "x1" ; then
  ENABLE_LIBCAP=True
  AC_DEFINE([GEOPM_ENABLE_LIBCAP], [ ], [Enables the use of libcap to do capabilities checks, otherwise direct syscall is used])
else
  ENABLE_LIBCAP=False
fi

AC_ARG_ENABLE([rawmsr],
  [AS_HELP_STRING([--disable-rawmsr], [Disable direct use of standard msr(4) device driver])],
[if test "x$enable_rawmsr" = "xno" ; then
  enable_rawmsr="0"
else
  enable_rawmsr="1"
fi
],
[enable_rawmsr="1"]
)

AC_SUBST([enable_rawmsr])
AM_CONDITIONAL([ENABLE_RAWMSR], [test "x$enable_rawmsr" = "x1"])

if test "x$enable_rawmsr" = "x1" ; then
  ENABLE_RAWMSR=True
  AC_DEFINE([GEOPM_ENABLE_RAWMSR], [ ], [Enable direct use of standard msr(4) device driver])
else
  ENABLE_RAWMSR=False
fi

AC_ARG_ENABLE([cpuid],
  [AS_HELP_STRING([--disable-cpuid], [Disable direct use of standard msr(4) device driver])],
[if test "x$enable_cpuid" = "xno" ; then
  enable_cpuid="0"
else
  enable_cpuid="1"
fi
],
[enable_cpuid="1"]
)

AC_SUBST([enable_cpuid])
AM_CONDITIONAL([ENABLE_CPUID], [test "x$enable_cpuid" = "x1"])

if test "x$enable_cpuid" = "x1" ; then
  ENABLE_CPUID=True
  AC_DEFINE([GEOPM_ENABLE_CPUID], [ ], [Enable use of the cpuid.h interfaces])
else
  ENABLE_CPUID=False
fi


AC_ARG_WITH([bash-completion-dir], [AS_HELP_STRING([--with-bash-completion-dir[=PATH]],
            [Install the bash auto-completion script in this directory. @<:@default=yes@:>@])])
if test "x$with_bash_completion_dir" = "x"; then
  bashcompdir=`pkg-config --define-variable=prefix='${prefix}' --variable=completionsdir bash-completion`
  if test "x$bashcompdir" = "x"; then
    bashcompdir="$sysconfdir/bash_completion.d"
  fi
else
  bashcompdir="$with_bash_completion_dir"
fi
AC_SUBST([bashcompdir])
AM_CONDITIONAL([ENABLE_BASH_COMPLETION], [test "x$with_bash_completion_dir" != "xno"])

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
fi
],
[enable_io_uring="1"]
)
AC_SUBST([enable_io_uring])
AM_CONDITIONAL([ENABLE_IO_URING], [test "x$enable_io_uring" = "x1"])

if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([GEOPM_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -Og -g"
  CXXFLAGS="$CXXFLAGS -Og -g"
fi
AC_SUBST([enable_debug])

if test "x$enable_coverage" = "x1" ; then
  AC_DEFINE([GEOPM_COVERAGE], [ ], [Enables test coverage reporting])
  AM_CFLAGS="$AM_CFLAGS --coverage"
  AM_CXXFLAGS="$AM_CXXFLAGS --coverage"
  AM_LDFLAGS="$AM_LDFLAGS --coverage -lgcov"
fi
AC_SUBST([enable_coverage])
AM_CONDITIONAL([ENABLE_COVERAGE], [test "x$enable_coverage" = "x1"])

if test "x$enable_asan" = "x1" ; then
  AM_CXXFLAGS="$AM_CXXFLAGS -fsanitize=address -fno-omit-frame-pointer -mllvm -asan-use-private-alias=1"
else
  AM_CXXFLAGS="$AM_CXXFLAGS $SANITIZE_CFI"
  AM_CFLAGS="$AM_CFLAGS $SANITIZE_CFI"
fi

AC_SUBST([enable_asan])
AM_CONDITIONAL([ENABLE_SANITIZERS], [test "x$enable_asan" = "x1"])

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
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_nvml/include"
  LD_LIBRARY_PATH="$with_nvml/lib64:$with_nvml/lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_nvml/lib -L$with_nvml/lib64"
fi

AC_ARG_WITH([dcgm], [AS_HELP_STRING([--with-dcgm=PATH],
            [specify directory for installed dcgm package.])])
if test "x$with_dcgm" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_dcgm/include"
  LD_LIBRARY_PATH="$with_dcgm/lib64:$with_dcgm/lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_dcgm/lib -L$with_dcgm/lib64"
fi

AC_ARG_WITH([levelzero], [AS_HELP_STRING([--with-levelzero=PATH],
            [specify directory for installed levelzero package.])])
if test "x$with_levelzero" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_levelzero/include"
  LD_LIBRARY_PATH="$with_levelzero/lib64:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_levelzero/lib64 -Wl,-rpath=$with_levelzero/lib64"
fi

AC_ARG_WITH([liburing], [AS_HELP_STRING([--with-liburing=PATH],
            [specify directory for the installed liburing package.])])
if test "x$with_liburing" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_liburing/include"
  LD_LIBRARY_PATH="$with_liburing/lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_liburing/lib"
fi
AC_ARG_WITH([liburing-include], [AS_HELP_STRING([--with-liburing-include=PATH],
            [specify directory for the installed liburing include files.])])
if test "x$with_liburing_include" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_liburing_include"
fi
AC_ARG_WITH([liburing-lib], [AS_HELP_STRING([--with-liburing-lib=PATH],
            [specify directory for the installed liburing library.])])
if test "x$with_liburing_lib" != x; then
  LD_LIBRARY_PATH="$with_liburing_lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_liburing_lib"
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

[AM_CFLAGS="$AM_CFLAGS -std=c99"]
[AM_CXXFLAGS="$AM_CXXFLAGS -std=c++17"]

# LDFLAGS must be set properly for the below AC_CHECK_LIB calls
[LDFLAGS_SAVE="$LDFLAGS"]
[LDFLAGS="$AM_LDFLAGS"]

# FLAGS must be set properly for the below AC_CHECK_HEADER calls
[CFLAGS_SAVE="$CFLAGS"]
[CXXFLAGS_SAVE="$CXXFLAGS"]
[CPPFLAGS_SAVE="$CPPFLAGS"]
[CFLAGS="$AM_CFLAGS"]
[CXXFLAGS="$AM_CXXFLAGS"]
[CPPFLAGS="$AM_CPPFLAGS"]

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

m4_include([m4/ax_check_compile_flag.m4])
AX_CHECK_COMPILE_FLAG([-fp-model="fast"], [FASTMATH="-fp-model=fast"], [FASTMATH="-ffast-math"], [], [])
AX_CHECK_COMPILE_FLAG([-fp-model="precise"], [NO_FASTMATH="-fp-model=precise"], [NO_FASTMATH="-fno-fast-math"], [], [])
AX_CHECK_COMPILE_FLAG([-fsanitize=cfi], [SANITIZE_CFI="-fsanitize=cfi"], [], [], [])

AC_CHECK_LIB([pthread], [pthread_mutex_init], [], [
    echo "missing libpthread: POSIX thread interface is required"
    exit -1])
AC_CHECK_HEADER([pthread.h], [], [
    echo "missing pthread.h: POSIX thread interface is required"
    exit -1])

AC_CHECK_LIB([z], [crc32], [], [
    echo "missing libz: Required for crc32 interface"
    exit -1])
AC_CHECK_HEADER([zlib.h], [], [
    echo "missing zlib.h: Required for crc32 interface"
    exit -1])

if test "x$enable_cpuid" = "x1" ; then
    AC_CHECK_HEADER([cpuid.h], [], [
        echo "missing cpuid.h: cpuid interface may be disabled with the --disable-cpuid option"
        exit -1])
fi

if test "x$enable_systemd" = "x1" ; then
    AC_CHECK_LIB([systemd], [sd_bus_open_system], [], [
        echo "missing libsystemd or does libsystemd does not support sd_bus interface, to disable ServiceIOGroup configure with --disable-systemd"
        exit -1])
    AC_CHECK_HEADER([systemd/sd-bus.h], [], [
        echo "missing systemd/sd-bus.h: sd_bus interface is required"
        exit -1])
fi

if test "x$enable_libcap" = "x1" ; then
    AC_CHECK_LIB([cap], [cap_get_bound], [], [
        echo "missing libcap, to disable configure with --disable-libcap"
        exit -1])
    AC_CHECK_HEADER([sys/capability.h], [], [
        echo "missing sys/capability.h: libcap development package is required"
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
    AC_DEFINE([GEOPM_HAS_IO_URING], [ ], [Enables liburing in GEOPM])
    AC_SEARCH_LIBS([io_uring_queue_init], [uring])
    AC_CHECK_LIB([uring], [io_uring_get_probe], [], [
        echo "missing liburing or version is too old: Configure with --with-liburing or --with-liburing-lib to specify the location or --disable-io-uring to avoid this dependency"
        exit -1])
    AC_CHECK_HEADER([liburing.h], [], [
        echo "missing liburing.h: Configure with --with-liburing or --with-liburing-include to specify the location or --disable-io-uring to avoid this dependency"
        exit -1],
        [#define _DEFAULT_SOURCE
         #define _POSIX_C_SOURCE 200809L
         #include <liburing.h>])

    # Newer versions of liburing provide an function to release probe
    # resources, but with older versions the user must call free(3) to
    # release the resources.  This check sets a preprocessor define
    # to identify which is required.
    AC_CHECK_LIB([uring], [io_uring_free_probe], [liburing_has_free=1], [liburing_has_free=0])
    if test "x$liburing_has_free" = "x1"; then
        AC_DEFINE([GEOPM_IO_URING_HAS_FREE], [ ], [Use io_uring_free_probe instead of free])
    fi
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

[LDFLAGS="$LDFLAGS_SAVE"]
[CFLAGS="$CFLAGS_SAVE"]
[CXXFLAGS="$CXXFLAGS_SAVE"]
[CPPFLAGS="$CPPFLAGS_SAVE"]

[AM_CFLAGS="$AM_CFLAGS -Wall -Wextra -Werror -Wno-unused-parameter -Wformat -Wformat-security $NO_FASTMATH -fPIC -flto -fPIE"]
[AM_CXXFLAGS="$AM_CXXFLAGS -Wall -Wextra -Werror -Wno-unused-parameter -Wformat -Wformat-security $NO_FASTMATH -fPIC -flto -fPIE"]
[AM_LDFLAGS="$AM_LDFLAGS -Wl,-z,relro,-z,now,-z,noexecstack -pie"]

AC_SUBST([AM_CFLAGS])
AC_SUBST([FASTMATH])
AC_SUBST([NO_FASTMATH])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_SUBST_FILE([JSON_SCHEMA_ACTIVE_SESSIONS])
JSON_SCHEMA_ACTIVE_SESSIONS=docs/json_schemas/active_sessions.schema.json
AC_SUBST_FILE([JSON_SCHEMA_CONST_CONFIG_IO])
JSON_SCHEMA_CONST_CONFIG_IO=docs/json_schemas/const_config_io.schema.json

GEOPM_SOURCE_DIR=$(readlink -f $srcdir)
AC_DEFINE_UNQUOTED([GEOPM_SOURCE_DIR], ["$GEOPM_SOURCE_DIR"], [Root of the GEOPM source directory])

AC_CONFIG_FILES([Makefile geopm-service.spec geopmdpy/version.py geopmdpy/schemas.py debian/changelog])
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
AC_MSG_RESULT([asan               : ${enable_asan}])
AC_MSG_RESULT([fuzz               : ${enable_fuzz}])
AC_MSG_RESULT([libcap             : ${enable_libcap}])
AC_MSG_RESULT([rawmsr             : ${enable_rawmsr}])
AC_MSG_RESULT([===============================================================================])
