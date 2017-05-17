#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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
AC_INIT([geopm],m4_esyscmd([tr -d '\n' < VERSION]),[christopher.m.cantalupo@intel.com])
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

LT_PREREQ([2.2.6])

AC_ARG_VAR([MPICC],
           [MPICC the MPI C compiler wrapper used to determine flags required for inclusion of MPI for C.])
AC_ARG_VAR([MPICXX],
           [MPICXX the MPI C++ compiler wrapper used to determine flags required for inclusion of MPI for C++.])
AC_ARG_VAR([MPIFC],
           [MPIFC the MPI Fortran compiler wrapper used to determine flags required for inclusion of MPI for Fortran.])
AC_ARG_VAR([MPIF77],
           [MPIF77 the MPI Fortran 77 compiler wrapper used to determine flags required for inclusion of MPI for Fortran 77.])

AC_ARG_WITH([hwloc], [AS_HELP_STRING([--with-hwloc=PATH],
            [specify directory for installed hwloc package.])])
if test "x$with_hwloc" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_hwloc/include"
  LD_LIBRARY_PATH="$with_hwloc/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_hwloc/lib"
fi
AC_ARG_WITH([hwloc-include], [AS_HELP_STRING([--with-hwloc-include=PATH],
            [specify directory for the installed hwloc include files.])])
if test "x$with_hwloc_include" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_hwloc_include"
fi
AC_ARG_WITH([hwloc-lib], [AS_HELP_STRING([--with-hwloc-lib=PATH],
            [specify directory for installed hwloc library])])
if test "x$with_hwloc_lib" != x; then
  LD_LIBRARY_PATH="$with_hwloc_lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_hwloc_lib"
fi

AC_ARG_WITH([json], [AS_HELP_STRING([--with-json=PATH],
            [specify directory for installed json-c package.])])
if test "x$with_json" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_json/include"
  LD_LIBRARY_PATH="$with_json/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_json/lib"
fi
AC_ARG_WITH([json-include], [AS_HELP_STRING([--with-json-include=PATH],
            [specify directory for the installed json-c include files.])])
if test "x$with_json_include" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_json_include"
fi
AC_ARG_WITH([json-lib], [AS_HELP_STRING([--with-json-lib=PATH],
            [specify directory for installed json-c library])])
if test "x$with_json_lib" != x; then
  LIBRARY_PATH="$with_json_lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_json_lib"
fi

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

AC_ARG_ENABLE([overhead],
  [AS_HELP_STRING([--enable-overhead], [Enable GEOPM to calculate and display time spent in GEOPM API calls])],
[if test "x$enable_overhead" = "xno" ; then
  enable_overhead="0"
else
  enable_overhead="1"
fi
],
[enable_overhead="0"]
)

if test "x$enable_openmp" = "xno" ; then
  enable_openmp="0"
else
  enable_openmp="1"
fi

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

if test "x$enable_overhead" = "x1" ; then
  AC_DEFINE([GEOPM_OVERHEAD], [ ], [Enables code for measuring overhead])
fi
AC_SUBST([enable_overhead])


AC_SUBST([enable_openmp])
AM_CONDITIONAL([ENABLE_OPENMP], [test "x$enable_openmp" = "x1"])

AC_ARG_WITH([mpi-bin], [AS_HELP_STRING([--with-mpi-bin=PATH],
            [specify directory for mpi compiler wrapper binaries])])
if test "x$with_mpi_bin" != x; then
  MPICC=$with_mpi_bin/mpicc
  MPICXX=$with_mpi_bin/mpicxx
  MPIFC=$with_mpi_bin/mpif90
  MPIF77=$with_mpi_bin/mpif77
fi

AC_ARG_WITH([mpicc], [AS_HELP_STRING([--with-mpicc=PATH],
            [specify path to mpi C compiler wrapper])])
if test "x$with_mpicc" != x; then
  MPICC=$with_mpicc
fi

AC_ARG_WITH([mpicxx], [AS_HELP_STRING([--with-mpicxx=PATH],
            [specify path to mpi C++ compiler wrapper])])
if test "x$with_mpicxx" != x; then
  MPICXX=$with_mpicxx
fi

AC_ARG_WITH([mpifc], [AS_HELP_STRING([--with-mpifc=PATH],
            [specify path to mpi Fortran compiler wrapper])])
if test "x$with_mpifc" != x; then
  MPIFC=$with_mpifc
fi

AC_ARG_WITH([mpif77], [AS_HELP_STRING([--with-mpif77=PATH],
            [specify path to mpi Fortran 77 compiler wrapper])])
if test "x$with_mpif77" != x; then
  MPIF77=$with_mpif77
fi

AC_ARG_ENABLE([mpi],
  [AS_HELP_STRING([--disable-mpi], [Do not build components that require MPI])],
[if test "x$enable_mpi" = "xno" ; then
  enable_mpi="0"
else
  enable_mpi="1"
fi
],
[enable_mpi="1"]
)
AC_SUBST([enable_mpi])
AM_CONDITIONAL([ENABLE_MPI], [test "x$enable_mpi" = "x1"])

AC_ARG_ENABLE([fortran],
  [AS_HELP_STRING([--disable-fortran], [Do not build fortran interface])],
[if test "x$enable_fortran" = "xno" ; then
  enable_fortran="0"
else
  enable_fortran="1"
fi
],
[enable_fortran="1"]
)
AC_SUBST([enable_fortran])
AM_CONDITIONAL([ENABLE_FORTRAN], [test "x$enable_fortran" = "x1"])

AC_ARG_ENABLE([doc],
  [AS_HELP_STRING([--disable-doc], [Do not create man pages with ronn])],
[if test "x$enable_doc" = "xno" ; then
  enable_doc="0"
else
  enable_doc="1"
fi
],
[enable_doc="1"]
)

AC_SUBST([enable_doc])
AM_CONDITIONAL([ENABLE_DOC], [test "x$enable_doc" = "x1"])


m4_include([m4/lx_find_mpi.m4])
LX_FIND_MPI()
AC_LANG_PUSH([C++])
LX_FIND_MPI()
AC_LANG_POP([C++])
if test "x$enable_fortran" = "x1"; then
AC_LANG_PUSH([Fortran])
LX_FIND_MPI()
AC_LANG_POP([Fortran])
AC_LANG_PUSH([Fortran 77])
LX_FIND_MPI()
AC_LANG_POP([Fortran 77])
fi
if test "x$enable_mpi" = "x1"; then
  AS_IF([test "x$have_C_mpi" == "xyes"],
         AC_MSG_NOTICE([Building with MPI C support]),
         AC_MSG_ERROR([Failed to determine MPI C build flags use --with-mpi-bin or --with-mpicc or --disable-mpi]))
  AS_IF([test "x$have_CXX_mpi" == "xyes"],
         AC_MSG_NOTICE([Building with MPI C++ support]),
         AC_MSG_ERROR([Failed to determine MPI C++ build flags use --with-mpi-bin or --with-mpicxx or --disable-mpi]))
if test "x$enable_fortran" = "x1"; then
  AS_IF([test "x$have_F_mpi" == "xyes"],
         AC_MSG_NOTICE([Building with MPI Fortran support]),
         AC_MSG_ERROR([Failed to determine MPI Fortran build flags use --with-mpi-bin or --with-mpicxx or --disable-mpi]))
  AS_IF([test "x$have_F77_mpi" == "xyes"],
         AC_MSG_NOTICE([Building with MPI Fortran 77 support]),
         AC_MSG_ERROR([Failed to determine MPI Fortran 77  build flags use --with-mpi-bin or --with-mpicxx or --disable-mpi]))
fi
fi

AC_LANG_PUSH([C++])
AC_OPENMP()
AC_LANG_POP([C++])


[MPI_FCFLAGS=$MPI_FFLAGS]
[MPI_FFLAGS=$MPI_F77FLAGS]
[MPI_LDFLAGS=$MPI_CLDFLAGS]
[MPI_LIBS=$MPI_CLIBS]
[MPI_FCLIBS=$MPI_FLIBS]
[MPI_FLIBS=$MPI_F77LIBS]

# WORK AROUND FOR SLES-12.
#
# The version of openmpi installed on SLES-12 has an mpicc wrapper
# which emits the include flag for the version of hwloc compiled into
# the mpi library.  This forces the geopm objects which use MPI to be
# derived from source files that include the openmpi version of the
# hwloc header.  The geopm objects which do not require MPI will
# derived from source files that include the system version in
# /usr/include which is always searched after all non-system -I
# options have been exhausted.
#
# https://gcc.gnu.org/onlinedocs/cpp/System-Headers.html#System-Headers
#
# The hwloc interface is based on enums that change frequently with
# version number (not a great feature) and we would like to use
# configure to cononicalize the hwloc interface with the
# GEOPM_HWLOC_HAS_* defines above.  This forces us to use one version
# of hwloc for MPI and non-MPI objects.  To enable this to be the
# system version we remove the include path for the openmpi version of
# hwloc compiled into the mpi library from the MPI_CLFAGS and
# MPI_CXXFLAGS.
#
# I would recommend that in the future SLES either not package hwloc
# or not allow the mpicc compiler wrapper to emit the include path for
# the openmpi versioned hwloc.  Alternatively the openmpi hwloc header
# should be versioned i.e. hwloc.h -> hwloc172.h.
#
[MPI_CFLAGS=`echo $MPI_CFLAGS | sed 's|-I/usr/lib64/mpi/gcc/openmpi/include/openmpi/opal/mca/hwloc/hwloc[0-9]*/hwloc/include||'`]
[MPI_CXXFLAGS=`echo $MPI_CXXFLAGS | sed 's|-I/usr/lib64/mpi/gcc/openmpi/include/openmpi/opal/mca/hwloc/hwloc[0-9]*/hwloc/include||'`]
[MPI_FCFLAGS=`echo $MPI_FCFLAGS | sed 's|-lmpigi||'`]
[MPI_FFFLAGS=`echo $MPI_FFFLAGS | sed 's|-lmpigi||'`]

AC_SUBST([MPI_FCFLAGS])
AC_SUBST([MPI_FFLAGS])
AC_SUBST([MPI_LDFLAGS])
AC_SUBST([MPI_LIBS])
AC_SUBST([MPI_FCLIBS])
AC_SUBST([MPI_FLIBS])

[EXTRA_CFLAGS="$EXTRA_CFLAGS $OPENMP_CXXFLAGS -std=c99 -msse4.2"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $OPENMP_CXXFLAGS -std=c++11 -msse4.2"]
[EXTRA_FFLAGS="$EXTRA_FFLAGS $OPENMP_CXXFLAGS -msse4.2"]
[EXTRA_FCFLAGS="$EXTRA_FCFLAGS $OPENMP_CXXFLAGS -msse4.2"]
[EXTRA_LDFLAGS="$EXTRA_LDFLAGS $OPENMP_CXXFLAGS"]


[LDFLAGS_SAVE="$LDFLAGS"]
[LDFLAGS="$EXTRA_LDFLAGS $LDFLAGS"]
[CFLAGS_SAVE="$CFLAGS"]
[CFLAGS="$EXTRA_CFLAGS $CFLAGS"]
[CXXFLAGS_SAVE="$CXXFLAGS"]
[CXXFLAGS="$EXTRA_CXXFLAGS $CXXFLAGS"]
[CPPFLAGS_SAVE="$CPPFLAGS"]
[CPPFLAGS="$EXTRA_CPPFLAGS $MPI_CXXFLAGS $CPPFLAGS"]

AC_CHECK_FUNC([sched_getaffinity],[enable_sched="1"],[enable_sched="0"])
AM_CONDITIONAL([ENABLE_SCHED], [test "x$enable_sched" = "x1"])

AC_SEARCH_LIBS([shm_open], [rt])
AC_SEARCH_LIBS([shm_unlink], [rt])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
if test "x$enable_fortran" = "x1"; then
AC_PROG_FC
AC_PROG_F77
fi
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
AC_CHECK_LIB([hwloc], [hwloc_topology_init], [], [
    echo "missing libhwloc: <http://www.open-mpi.org/projects/hwloc> use --with-hwloc or --with-hwloc-lib"
    exit -1])
AC_CHECK_LIB([hwloc], [hwloc_topology_dup], [
    AC_DEFINE([GEOPM_HWLOC_HAS_TOPOLOGY_DUP], [1], [hwloc supports hwloc_topology_dup.])], [])
AC_CHECK_HEADER([hwloc.h], [], [
    echo "missing hwloc.h: <http://www.open-mpi.org/projects/hwloc> use --with-hwloc or --with-hwloc-include"
    exit -1])
AC_CHECK_LIB([json-c], [json_tokener_parse], [], [
    echo "missing libjson-c: <https://github.com/json-c/json-c/wiki> use --with-json or --with-json-lib"
    exit -1])
AC_CHECK_HEADER([json-c/json.h], [], [
    echo "missing json-c/json.h: <https://github.com/json-c/json-c/wiki> use --with-json or --with-json-include"
    exit -1])

AC_CHECK_HEADER([xmmintrin.h], [AC_DEFINE([GEOPM_HAS_XMMINTRIN], [1], [xmmintrin.h is available])], [])
AC_CHECK_HEADER([ompt.h], [AC_DEFINE([GEOPM_HAS_OMPT], [1], [ompt.h is available]) [has_ompt="1"]], [has_ompt="0"])

AC_ARG_ENABLE([ompt],
  [AS_HELP_STRING([--enable-ompt], [Use OpenMP Tool interface])],
[if test "x$enable_ompt" = "xno"; then
  enable_ompt="0"
else
  if test "x$enable_openmp" = "x0"; then
      enable_ompt="0"
      echo "Warning: opmt requested, but openmp not found"
  fi
  enable_ompt="1"
fi
],
[enable_ompt="$has_ompt"]
)

if test "x$enable_ompt" = "x1" ; then
  AC_DEFINE([GEOPM_ENABLE_OMPT], [ ], [Enables OpenMP Tool callbacks.])
fi
AM_SUBST([enable_ompt])
AM_CONDITIONAL([ENABLE_OMPT], [test "x$enable_ompt" = "x1"])
AM_CONDITIONAL([HAS_OMPT], [test "x$has_ompt" = "x1"])

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
AC_CHECK_FUNCS([clock_gettime dup2 floor getcwd memset mkfifo pow sqrt strdup strstr])

AC_MSG_CHECKING([that hwloc supports HWLOC_OBJ_SOCKET])
AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([[#include <hwloc.h>]],
            [[hwloc_topology_t m_topo;
            hwloc_topology_init(&m_topo);
            hwloc_topology_load(m_topo);
            int depth = hwloc_get_type_depth(m_topo, HWLOC_OBJ_SOCKET);]])
],[
    AC_MSG_RESULT(yes)
    AC_DEFINE([GEOPM_HWLOC_HAS_SOCKET], [1], [hwloc supports HWLOC_OBJ_SOCKET.])
],[
    AC_MSG_RESULT(no)
])

AC_MSG_CHECKING([that hwloc supports HWLOC_OBJ_L2CACHE])
AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([[#include <hwloc.h>]],
            [[hwloc_topology_t m_topo;
            hwloc_topology_init(&m_topo);
            hwloc_topology_load(m_topo);
            int size = hwloc_get_nbobjs_by_type(m_topo, HWLOC_OBJ_L2CACHE);]])
],[
    AC_MSG_RESULT(yes)
    AC_DEFINE([GEOPM_HWLOC_HAS_L2CACHE], [1], [hwloc supports HWLOC_OBJ_L2CACHE.])
],[
    AC_MSG_RESULT(no)
])

AM_CONDITIONAL([HAVE_IFORT], [test ! -z "$FC" && $FC --version | grep ifort > /dev/null])
AM_CONDITIONAL([HAVE_ICC], [test ! -z "$CC" && $CC --version | grep icc > /dev/null])
AM_CONDITIONAL([HAVE_ICPC], [test ! -z "$CXX" && $CXX --version | grep icpc > /dev/null])

AC_MSG_CHECKING([for support for MPI-3 standard])
AC_COMPILE_IFELSE([
    AC_LANG_SOURCE(
        [[#include <mpi.h>
          int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {return 0;}]])
],[
    AC_MSG_RESULT(yes)
    AC_DEFINE([GEOPM_MPI_CONST], [const], [MPI interfaces use const qualifier.])
    AC_DEFINE([GEOPM_MPI_CONST_CAST(t)], [], [MPI interfaces use const qualifier.])
    AC_DEFINE([GEOPM_ENABLE_MPI3], [], [MPI-3 support enabled.])
],[
    AC_MSG_RESULT(no)
    AC_DEFINE([GEOPM_MPI_CONST], [], [MPI interfaces do not use const qualifier.])
    AC_DEFINE([GEOPM_MPI_CONST_CAST(t)], [const_cast<t>], [MPI interfaces do not use const qualifier.])
])

[CFLAGS="$CFLAGS -Wall -Werror -fno-delete-null-pointer-checks"]
AC_MSG_CHECKING([for support for -fno-delete-null-pointer-checks])
AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([[#include <stdio.h>]],
          [[printf("Hello World.\n");]])
],[
    AC_MSG_RESULT(yes)
    AM_CONDITIONAL([GEOPM_DISABLE_NULL_PTR], [true])
],[
    AC_MSG_RESULT(no)
    AM_CONDITIONAL([GEOPM_DISABLE_NULL_PTR], [false])
])

[LDFLAGS="$LDFLAGS_SAVE"]
[CFLAGS="$CFLAGS_SAVE"]
[CXXFLAGS="$CXXFLAGS_SAVE"]
[CPPFLAGS="$CPPFLAGS_SAVE"]

[EXTRA_CFLAGS="$EXTRA_CFLAGS -Wall -Werror"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -Wall -Werror"]
[EXTRA_FFLAGS="$EXTRA_FFLAGS -Wall -Werror -fno-range-check"]
[EXTRA_FCFLAGS="$EXTRA_FCFLAGS -Wall -Werror -fno-range-check"]

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

AC_CONFIG_FILES([Makefile])
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
AC_MSG_RESULT([MPI_CFLAGS         : ${MPI_CFLAGS}])
AC_MSG_RESULT([CXX                : ${CXX}])
AC_MSG_RESULT([CXXFLAGS           : ${CXXFLAGS}])
AC_MSG_RESULT([AM_CXXFLAGS        : ${AM_CXXFLAGS}])
AC_MSG_RESULT([MPI_CXXFLAGS       : ${MPI_CXXFLAGS}])
AC_MSG_RESULT([F77                : ${F77}])
AC_MSG_RESULT([FFLAGS             : ${FFLAGS}])
AC_MSG_RESULT([AM_FFLAGS          : ${AM_FFLAGS}])
AC_MSG_RESULT([MPI_FFLAGS         : ${MPI_FFLAGS}])
AC_MSG_RESULT([FC                 : ${FC}])
AC_MSG_RESULT([FCFLAGS            : ${FCFLAGS}])
AC_MSG_RESULT([AM_FCFLAGS         : ${AM_FCFLAGS}])
AC_MSG_RESULT([MPI_FCFLAGS        : ${MPI_FCFLAGS}])
AC_MSG_RESULT([LDFLAGS            : ${LDFLAGS}])
AC_MSG_RESULT([AM_LDFLAGS         : ${AM_LDFLAGS}])
AC_MSG_RESULT([LD_LIBRARY_PATH    : ${LD_LIBRARY_PATH}])
AC_MSG_RESULT([LIBS               : ${LIBS}])
AC_MSG_RESULT([FLIBS              : ${FLIBS}])
AC_MSG_RESULT([FCLIBS             : ${FCLIBS}])
AC_MSG_RESULT([MPI_LDFLAGS        : ${MPI_LDFLAGS}])
AC_MSG_RESULT([MPI_LIBS           : ${MPI_LIBS}])
AC_MSG_RESULT([MPI_CXXLIBS        : ${MPI_CXXLIBS}])
AC_MSG_RESULT([MPI_FLIBS          : ${MPI_FLIBS}])
AC_MSG_RESULT([MPI_FCLIBS         : ${MPI_FCLIBS}])
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
AC_MSG_RESULT([overhead           : ${enable_overhead}])
AC_MSG_RESULT([mpi                : ${enable_mpi}])
AC_MSG_RESULT([openmp             : ${enable_openmp}])
AC_MSG_RESULT([fortran            : ${enable_fortran}])
AC_MSG_RESULT([doc                : ${enable_doc}])
AC_MSG_RESULT([ompt               : ${enable_ompt}])
AC_MSG_RESULT([===============================================================================])
