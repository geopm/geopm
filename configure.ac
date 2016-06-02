#  Copyright (c) 2015, 2016, Intel Corporation
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
  CPPFLAGS="$AM_CPPFLAGS -I$with_hwloc/include"
  LD_LIBRARY_PATH="$with_hwloc/lib:$LD_LIBRARY_PATH"
  LDFLAGS="$AM_LDFLAGS -L$with_hwloc/lib"
fi
AC_ARG_WITH([hwloc-include], [AS_HELP_STRING([--with-hwloc-include=PATH],
            [specify directory for the installed hwloc include files.])])
if test "x$with_hwloc_include" != x; then
  CPPFLAGS="$AM_CPPFLAGS -I$with_hwloc_include"
fi
AC_ARG_WITH([hwloc-lib], [AS_HELP_STRING([--with-hwloc-lib=PATH],
            [specify directory for installed hwloc library])])
if test "x$with_hwloc_lib" != x; then
  LD_LIBRARY_PATH="$with_hwloc_lib:$LD_LIBRARY_PATH"
  LDFLAGS="$AM_LDFLAGS -L$with_hwloc_lib"
fi

AC_ARG_WITH([json], [AS_HELP_STRING([--with-json=PATH],
            [specify directory for installed json-c package.])])

if test "x$with_json" != x; then
  CPPFLAGS="$AM_CPPFLAGS -I$with_json/include"
  LD_LIBRARY_PATH="$with_json/lib:$LD_LIBRARY_PATH"
  LDFLAGS="$AM_LDFLAGS -L$with_json/lib"
fi
AC_ARG_WITH([json-include], [AS_HELP_STRING([--with-json-include=PATH],
            [specify directory for the installed json-c include files.])])
if test "x$with_json_include" != x; then
  CPPFLAGS="$AM_CPPFLAGS -I$with_json_include"
fi
AC_ARG_WITH([json-lib], [AS_HELP_STRING([--with-json-lib=PATH],
            [specify directory for installed json-c library])])
if test "x$with_json_lib" != x; then
  LIBRARY_PATH="$with_json_lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_json_lib"
fi

AC_ARG_WITH([mpi-bin], [AS_HELP_STRING([--with-mpi-bin=PATH],
            [specify directory for mpi compiler wrapper binaries])])
if test "x$with_mpi_bin" != x; then
  MPICC=$with_mpi_bin/mpicc
  MPICXX=$with_mpi_bin/mpicxx
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
  [AS_HELP_STRING([--disable-fortan], [Do not build fortran interface])],
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

AC_ARG_ENABLE([pmpi],
  [AS_HELP_STRING([--disable-pmpi], [Do not compile PMPI wrappers into libgeopm.so])],
[if test "x$enable_pmpi" = "xno" ; then
  enable_pmpi="0"
else
  enable_pmpi="1"
fi
],
[enable_pmpi="1"]
)
if test "x$enable_pmpi" = "x1" ; then
  AC_DEFINE([GEOPM_PMPI], [ ], [Enables use of geopm's  PMPI wrappers])
fi

AC_SUBST([enable_pmpi])
AM_CONDITIONAL([ENABLE_PMPI], [test "x$enable_pmpi" = "x1"])

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
AC_LANG_PUSH([C++])
AC_OPENMP()
AC_LANG_POP([C++])
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
AC_CHECK_HEADER([hwloc.h], [], [
    echo "missing hwloc.h: <http://www.open-mpi.org/projects/hwloc> use --with-hwloc or --with-hwloc-include"
    exit -1])
AC_CHECK_LIB([json-c], [json_tokener_parse], [], [
    echo "missing libjson-c: <https://github.com/json-c/json-c/wiki> use --with-json or --with-json-lib"
    exit -1])
AC_CHECK_HEADER([json-c/json.h], [], [
    echo "missing json-c/json.h: <https://github.com/json-c/json-c/wiki> use --with-json or --with-json-include"
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
  CFLAGS="$CFLAGS --coverage"
  CXXFLAGS="$CXXFLAGS --coverage"
  LDFLAGS="$LDFLAGS --coverage -lgcov"
fi
AC_SUBST([enable_coverage])

AC_SUBST([enable_openmp])
AM_CONDITIONAL([ENABLE_OPENMP], [test "x$enable_openmp" = "x1"])

AC_MSG_CHECKING([that hwloc supports HWLOC_OBJ_TYPE_PACKAGE])
AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM([[#include <hwloc.h>]],
            [[hwloc_topology_t m_topo;
            hwloc_topology_init(&m_topo);
            hwloc_topology_load(m_topo);
            int depth = hwloc_get_type_depth(m_topo, HWLOC_OBJ_PACKAGE);]])
],[
    AC_MSG_RESULT(yes)
],[
    AC_MSG_RESULT(no)
    AC_DEFINE([HWLOC_OBJ_PACKAGE], [HWLOC_OBJ_SOCKET], [Define HWLOC_OBJ_PACKAGE to HWLOC_OBJ_SOCKET if you have an older version of hwloc.])
])

AM_CONDITIONAL([HAVE_IFORT], [test ! -z "$FC" && $FC --version | grep ifort > /dev/null])
AM_CONDITIONAL([HAVE_ICC], [test ! -z "$CC" && $CC --version | grep icc > /dev/null])
AM_CONDITIONAL([HAVE_ICPC], [test ! -z "$CXX" && $CXX --version | grep icpc > /dev/null])

AM_CFLAGS="${AM_CFLAGS} ${OPENMP_CXXFLAGS} -Wall -Werror -std=c99 -msse4.2"
AM_CXXFLAGS="${AM_CXXFLAGS} ${OPENMP_CXXFLAGS} -Wall -Werror -std=c++11 -msse4.2"
AM_FFLAGS="${AM_FFLAGS} ${OPENMP_CXXFLAGS} -Wall -Werror -msse4.2"
AM_FCFLAGS="${AM_FCFLAGS} ${OPENMP_CXXFLAGS} -Wall -Werror -msse4.2"
AM_LDFLAGS="${AM_LDFLAGS} ${OPENMP_CXXFLAGS}"

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_FFLAGS])
AC_SUBST([AM_FCFLAGS])
AC_SUBST([AM_LDFLAGS])

MPI_FCFLAGS=$MPI_FFLAGS
MPI_FFLAGS=$MPI_F77FLAGS
MPI_LDFLAGS=$MPI_CLDFLAGS
MPI_LIBS=$MPI_CLIBS
MPI_FCLIBS=$MPI_FLIBS
MPI_FLIBS=$MPI_F77LIBS

AC_SUBST([MPI_FCFLAGS])
AC_SUBST([MPI_FFLAGS])
AC_SUBST([MPI_LDFLAGS])
AC_SUBST([MPI_LIBS])
AC_SUBST([MPI_FCLIBS])
AC_SUBST([MPI_FLIBS])

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
AC_MSG_RESULT([mpi                : ${enable_mpi}])
AC_MSG_RESULT([openmp             : ${enable_openmp}])
AC_MSG_RESULT([fortran            : ${enable_fortran}])
AC_MSG_RESULT([pmpi               : ${enable_pmpi}])
AC_MSG_RESULT([===============================================================================])
