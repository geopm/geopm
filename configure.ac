#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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

geopm_abi_version=1:0:0
AC_SUBST(geopm_abi_version)
AC_DEFINE_UNQUOTED([GEOPM_ABI_VERSION], ["$geopm_abi_version"], [GEOPM shared object version])

LT_PREREQ([2.2.6])

AC_ARG_VAR([MPICC],
           [MPICC the MPI C compiler wrapper used to determine flags required for inclusion of MPI for C.])
AC_ARG_VAR([MPICXX],
           [MPICXX the MPI C++ compiler wrapper used to determine flags required for inclusion of MPI for C++.])
AC_ARG_VAR([MPIFC],
           [MPIFC the MPI Fortran compiler wrapper used to determine flags required for inclusion of MPI for Fortran.])
AC_ARG_VAR([MPIF77],
           [MPIF77 the MPI Fortran 77 compiler wrapper used to determine flags required for inclusion of MPI for Fortran 77.])

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

AC_ARG_ENABLE([beta],
  [AS_HELP_STRING([--enable-beta], [Enable beta features of GEOPM that have intefaces expected to change])],
[if test "x$enable_beta" = "xno" ; then
  enable_beta="0"
else
  enable_beta="1"
fi
],
[enable_beta="0"]
)
AC_SUBST([enable_beta])
AM_CONDITIONAL([ENABLE_BETA], [test "x$enable_beta" = "x1"])

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

AC_ARG_WITH([sqlite3], [AS_HELP_STRING([--with-sqlite3=PATH],
            [specify directory for installed sqlite3 package.])])
if test "x$with_sqlite3" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_sqlite3/include"
  LD_LIBRARY_PATH="$with_sqlite3/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_sqlite3/lib"
fi
AC_ARG_WITH([sqlite3-include], [AS_HELP_STRING([--with-sqlite3-include=PATH],
            [specify directory for the installed sqlite3 include files.])])
if test "x$with_sqlite3_include" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_sqlite3_include"
fi
AC_ARG_WITH([sqlite3-lib], [AS_HELP_STRING([--with-sqlite3-lib=PATH],
            [specify directory for installed sqlite3 library])])
if test "x$with_sqlite3_lib" != x; then
  LD_LIBRARY_PATH="$with_sqlite3_lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_sqlite3_lib"
fi

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

# The block of code for geopmd-local must precede the blocks below for
# with-geopmpd* so that the with-geopmd* options take precedence over
# using the local build.
AC_ARG_ENABLE([geopmd-local],
  [AS_HELP_STRING([--disable-geopmd-local],
  [Use installed geopmd package, do not use in place build of the service directory.])],
[if test "x$enable_geopmd_local" = "xno" ; then
  enable_geopmd_local="0"
else
  enable_geopmd_local="1"
fi
],
[enable_geopmd_local="1"]
)

if test "x$enable_geopmd_local" = "x1"; then
  GEOPMD_INCLUDE="$srcdir/service/src"
  GEOPMD_LIBDIR="$srcdir/service/.libs"
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$GEOPMD_INCLUDE"
  LD_LIBRARY_PATH="$GEOPMD_LIBDIR:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$GEOPMD_LIBDIR"
  AC_SUBST([GEOPMD_LIBDIR])
  AC_SUBST([GEOPMD_INCLUDE])
fi

# This must stay below the geopm-local block above
AC_ARG_WITH([geopmd], [AS_HELP_STRING([--with-geopmd=PATH],
            [specify directory for installed geopmd package.])])
if test "x$with_geopmd" != x; then
  GEOPMD_INCLUDE=$with_geopmd/include
  GEOPMD_LIBDIR=$with_geopmd/lib
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$GEOPMD_INCLUDE"
  LD_LIBRARY_PATH="$GEOPMD_LIBDIR:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$GEOPMD_LIBDIR"
  AC_SUBST([GEOPMD_LIBDIR])
  AC_SUBST([GEOPMD_INCLUDE])
fi
AC_ARG_WITH([geopmd-include], [AS_HELP_STRING([--with-geopmd-include=PATH],
            [specify directory for the installed geopmd include files.])])
if test "x$with_geopmd_include" != x; then
  GEOPMD_INCLUDE=$with_geopmd_include
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$GEOPMD_INCLUDE"
  AC_SUBST([GEOPMD_INCLUDE])
fi
AC_ARG_WITH([geopmd-lib], [AS_HELP_STRING([--with-geopmd-lib=PATH],
            [specify directory for installed geopmd library])])
if test "x$with_geopmd_lib" != x; then
  GEOPMD_LIBDIR=$with_geopmd_lib
  LD_LIBRARY_PATH="$GEOPMD_LIBDIR:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$GEOPMD_LIBDIR"
  AC_SUBST([GEOPMD_LIBDIR])
fi


AC_ARG_WITH([libelf], [AS_HELP_STRING([--with-libelf=PATH],
            [specify directory for installed libelf package.])])
if test "x$with_libelf" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_libelf/include"
  LD_LIBRARY_PATH="$with_libelf/lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_libelf/lib"
fi
AC_ARG_WITH([libelf-include], [AS_HELP_STRING([--with-libelf-include=PATH],
            [specify directory for the installed libelf include files.])])
if test "x$with_libelf_include" != x; then
  EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -I$with_libelf_include"
fi
AC_ARG_WITH([libelf-lib], [AS_HELP_STRING([--with-libelf-lib=PATH],
            [specify directory for installed libelf library.])])
if test "x$with_libelf_lib" != x; then
  LD_LIBRARY_PATH="$with_libelf_lib:$LD_LIBRARY_PATH"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$with_libelf_lib"
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
AC_OPENMP
AC_LANG_POP([C++])

if test "x$enable_openmp" = "xno" ; then
  enable_openmp="0"
else
  enable_openmp="1"
fi

AC_SUBST([enable_openmp])
AM_CONDITIONAL([ENABLE_OPENMP], [test "x$enable_openmp" = "x1"])

[MPI_FCFLAGS=$MPI_FFLAGS]
[MPI_FFLAGS=$MPI_F77FLAGS]
[MPI_LDFLAGS=$MPI_CLDFLAGS]
[MPI_LIBS=$MPI_CLIBS]
[MPI_FCLIBS=$MPI_FLIBS]
[MPI_FLIBS=$MPI_F77LIBS]

[MPI_FCFLAGS=`echo $MPI_FCFLAGS | sed 's|-lmpigi||'`]
[MPI_FFFLAGS=`echo $MPI_FFFLAGS | sed 's|-lmpigi||'`]

AC_SUBST([MPI_FCFLAGS])
AC_SUBST([MPI_FFLAGS])
AC_SUBST([MPI_LDFLAGS])
AC_SUBST([MPI_LIBS])
AC_SUBST([MPI_FCLIBS])
AC_SUBST([MPI_FLIBS])

[EXTRA_CFLAGS="$EXTRA_CFLAGS $OPENMP_CXXFLAGS -std=c99 -msse4.2"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $OPENMP_CXXFLAGS -std=c++17 -msse4.2"]
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
AC_SEARCH_LIBS([dlopen], [dl dld])

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

if test "x$enable_beta" = "x1" ; then
  AC_CHECK_LIB([sqlite3], [sqlite3_open], [], [
      echo "missing libsqlite3: <https://www.sqlite.org> use --with-sqlite3 or --with-sqlite3-lib"
      exit -1])
  AC_CHECK_HEADER([sqlite3.h], [], [
      echo "missing sqlite3.h: <https://www.sqlite.org> use --with-sqlite3 or --with-sqlite3-include"
      exit -1])
fi

AC_CHECK_HEADER([omp-tools.h], [AC_DEFINE([GEOPM_HAS_OMPT], [1], [omp-tools.h is available]) [has_ompt="1"]], [has_ompt="0"])

AC_ARG_ENABLE([ompt],
  [AS_HELP_STRING([--enable-ompt], [Use OpenMP Tool interface])],
[if test "x$enable_ompt" = "xno"; then
  enable_ompt="0"
else
  if test "x$enable_openmp" = "x0"; then
      enable_ompt="0"
      echo "Warning: ompt requested, but openmp not found"
  else
      enable_ompt="1"
  fi
fi
],
[if test "x$enable_openmp" = "x1"; then
     enable_ompt="$has_ompt"
 else
     enable_ompt="0"
 fi
])

if test "x$enable_ompt" = "x1" ; then
  AC_DEFINE([GEOPM_ENABLE_OMPT], [ ], [Enables OpenMP Tool callbacks.])
fi

AC_SUBST([enable_ompt])
AM_CONDITIONAL([ENABLE_OMPT], [test "x$enable_ompt" = "x1"])
AM_CONDITIONAL([HAS_OMPT], [test "x$has_ompt" = "x1"])

AC_SEARCH_LIBS([geopm_pio_read_signal], [geopmd])
AC_CHECK_LIB([geopmd], [geopm_pio_read_signal], [], [
    echo "missing libgeopmd: Install the geopm-service and use --with-geopmd or --with-geopmd-lib to specify location"
    exit -1])
AC_CHECK_HEADER([geopm_pio.h], [], [
    echo "missing geopm_pio.h: Install the geopm-service and use --with-geopmd or --with-geopmd-include to specify location"
    exit -1])

AC_SEARCH_LIBS([elf_begin], [elf])
AC_CHECK_LIB([elf], [elf_begin], [], [
    echo "missing libelf: ELF library is required, use --with-libelf or --with-libelf-lib to specify location"
    exit -1])
AC_CHECK_HEADER([libelf.h], [], [
    echo "missing libelf.h: ELF library is required, use --with-libelf or --with-libelf-include to specify location"
    exit -1])
AC_CHECK_HEADER([gelf.h], [], [
    echo "missing gelf.h: ELF library is required, use --with-libelf or --with-libelf-include to specify location"
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
AC_CHECK_FUNCS([clock_gettime dup2 floor getcwd memset mkfifo pow sqrt strdup strstr])

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

AM_CONDITIONAL([HAVE_GFORTRAN], [test ! -z "$FC" && $FC --version | grep "GNU Fortran" > /dev/null])
AM_CONDITIONAL([HAVE_IFORT], [test ! -z "$FC" && $FC --version | grep ifort > /dev/null])
AM_CONDITIONAL([HAVE_ICC], [test ! -z "$CC" && $CC --version | grep icc > /dev/null])
AM_CONDITIONAL([HAVE_ICPC], [test ! -z "$CXX" && $CXX --version | grep icpc > /dev/null])

if test ! -z "$CC" && $CC --version | grep icc > /dev/null; then
   AC_DEFINE([GEOPM_ENABLE_MKL], [], [Compiling with Intel MKL support.])
fi

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
    AM_CONDITIONAL([GEOPM_ENABLE_MPI3], [true])
],[
    AC_MSG_RESULT(no)
    AC_DEFINE([GEOPM_MPI_CONST], [], [MPI interfaces do not use const qualifier.])
    AC_DEFINE([GEOPM_MPI_CONST_CAST(t)], [const_cast<t>], [MPI interfaces do not use const qualifier.])
    AM_CONDITIONAL([GEOPM_ENABLE_MPI3], [false])
])

[LDFLAGS="$LDFLAGS_SAVE"]
[CFLAGS="$CFLAGS_SAVE"]
[CXXFLAGS="$CXXFLAGS_SAVE"]
[CPPFLAGS="$CPPFLAGS_SAVE"]

[EXTRA_CFLAGS="$EXTRA_CFLAGS -Wall -Werror"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -Wall -Werror"]
if test ! -z "$FC" && $FC --version | grep "GNU Fortran" > /dev/null; then
[EXTRA_FFLAGS="$EXTRA_FFLAGS -fno-range-check"]
[EXTRA_FCFLAGS="$EXTRA_FCFLAGS -fno-range-check"]
else
[EXTRA_FFLAGS="$EXTRA_FFLAGS"]
[EXTRA_FCFLAGS="$EXTRA_FCFLAGS"]
fi

m4_include([m4/ax_check_compile_flag.m4])
AX_CHECK_COMPILE_FLAG([-fp-model="fast"], [FASTMATH="-fp-model=fast"], [FASTMATH="-ffast-math"], [], [])
AX_CHECK_COMPILE_FLAG([-fp-model="precise"], [NO_FASTMATH="-fp-model=precise"], [NO_FASTMATH="-fno-fast-math"], [], [])

[EXTRA_CFLAGS="$EXTRA_CFLAGS $NO_FASTMATH"]
[EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $NO_FASTMATH"]

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

AC_SUBST_FILE([CHANGELOG])
CHANGELOG=ChangeLog

AC_SUBST_FILE([BLURB])
BLURB=BLURB

if test "x$enable_beta" = "x1"; then
  ENABLE_BETA=True
  AC_DEFINE([GEOPM_ENABLE_BETA], [ ], [Enables beta feature implementations.])
else
  ENABLE_BETA=False
fi
AC_SUBST([ENABLE_BETA])

GEOPM_SOURCE_DIR=$(readlink -f $srcdir)
AC_DEFINE_UNQUOTED([GEOPM_SOURCE_DIR], ["$GEOPM_SOURCE_DIR"], [Root of the GEOPM source directory])

AC_CONFIG_FILES([Makefile specs/geopm-runtime.spec specs/geopm-dudley.spec specs/geopm-ohpc.spec specs/geopm-theta.spec scripts/geopmpy/version.py])
AC_CONFIG_FILES([test/geopm_test.sh], [chmod a+x test/geopm_test.sh])
AC_CONFIG_FILES([scripts/test/geopmpy_test.sh], [chmod a+x scripts/test/geopmpy_test.sh])
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
AC_MSG_RESULT([bashcompdir        : ${bashcompdir}])
AC_MSG_RESULT([GEOPM_CONFIG_PATH  : ${GEOPM_CONFIG_PATH}])
AC_MSG_RESULT([])
AC_MSG_RESULT([debug              : ${enable_debug}])
AC_MSG_RESULT([coverage           : ${enable_coverage}])
AC_MSG_RESULT([overhead           : ${enable_overhead}])
AC_MSG_RESULT([mpi                : ${enable_mpi}])
AC_MSG_RESULT([openmp             : ${enable_openmp}])
AC_MSG_RESULT([fortran            : ${enable_fortran}])
AC_MSG_RESULT([ompt               : ${enable_ompt}])
AC_MSG_RESULT([beta               : ${enable_beta}])
AC_MSG_RESULT([===============================================================================])
