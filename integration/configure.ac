#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# 2.62 required for OpenMP macro and others
AC_PREREQ([2.62])
AC_INIT([geopm-integration],m4_esyscmd([tr -d '\n' < VERSION]),[christopher.m.cantalupo@intel.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects tar-pax] foreign)
AC_CONFIG_HEADERS([config.h])

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h stdint.h stdlib.h string.h sys/time.h unistd.h])
LT_PREREQ([2.2.6])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

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


AC_ARG_ENABLE([beta],
  [AS_HELP_STRING([--enable-beta], [Enable beta features of GEOPM that have interfaces expected to change])],
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


AC_ARG_WITH([sqlite3], [AS_HELP_STRING([--with-sqlite3=PATH],
            [specify directory for installed sqlite3 package.])])
if test "x$with_sqlite3" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_sqlite3/include"
  LD_LIBRARY_PATH="$with_sqlite3/lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_sqlite3/lib"
fi
AC_ARG_WITH([sqlite3-include], [AS_HELP_STRING([--with-sqlite3-include=PATH],
            [specify directory for the installed sqlite3 include files.])])
if test "x$with_sqlite3_include" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_sqlite3_include"
fi
AC_ARG_WITH([sqlite3-lib], [AS_HELP_STRING([--with-sqlite3-lib=PATH],
            [specify directory for installed sqlite3 library])])
if test "x$with_sqlite3_lib" != x; then
  LD_LIBRARY_PATH="$with_sqlite3_lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_sqlite3_lib"
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

AC_ARG_ENABLE([mpi],
  [AS_HELP_STRING([--disable-mpi], [Do not link to MPI])],
[if test "x$enable_mpi" = "xno" ; then
  enable_mpi="0"
else
  enable_mpi="1"
fi
],
[enable_mpi="1"]
)

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

if test "x$enable_debug" = "x1" ; then
  AC_DEFINE([GEOPM_DEBUG], [ ], [Enables code for debugging])
  CFLAGS="$CFLAGS -Og -g"
  CXXFLAGS="$CXXFLAGS -Og -g"
fi
AC_SUBST([enable_debug])

AC_SUBST([enable_mpi])
AM_CONDITIONAL([ENABLE_MPI], [test "x$enable_mpi" = "x1"])
AC_SUBST([enable_fortran])
AM_CONDITIONAL([ENABLE_FORTRAN], [test "x$enable_fortran" = "x1"])

# The block of code for geopmd-local must precede the blocks below for
# with-geopmpd* so that the with-geopmd* options take precedence over
# using the local build.
AC_ARG_ENABLE([geopm-local],
  [AS_HELP_STRING([--disable-geopm-local],
  [Use installed geopmd package, do not use in place build of the service directory.])],
[if test "x$enable_geopmd_local" = "xno" ; then
  enable_geopm_local="0"
else
  enable_geopm_local="1"
fi
],
[enable_geopm_local="1"]
)

if test "x$enable_geopm_local" = "x1"; then
  GEOPMD_INCLUDE="$srcdir/../libgeopmd/include"
  GEOPMD_LIBDIR="$srcdir/../libgeopmd/.libs"
  GEOPM_INCLUDE="$srcdir/../libgeopm/include"
  GEOPM_LIBDIR="$srcdir/../libgeopm/.libs"
  AM_CPPFLAGS="$AM_CPPFLAGS -I$GEOPMD_INCLUDE -I$GEOPM_INCLUDE"
  LD_LIBRARY_PATH="$GEOPMD_LIBDIR:$GEOPM_LIBDIR:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$GEOPMD_LIBDIR -L$GEOPM_LIBDIR"
  AC_SUBST([GEOPMD_LIBDIR])
  AC_SUBST([GEOPMD_INCLUDE])
  AC_SUBST([GEOPM_LIBDIR])
  AC_SUBST([GEOPM_INCLUDE])
fi

# This must stay below the geopm-local block above
AC_ARG_WITH([geopmd], [AS_HELP_STRING([--with-geopmd=PATH],
            [specify directory for installed geopmd package.])])
if test "x$with_geopmd" != x; then
  GEOPMD_INCLUDE=$with_geopmd/include
  GEOPMD_LIBDIR=$with_geopmd/lib
  AM_CPPFLAGS="$AM_CPPFLAGS -I$GEOPMD_INCLUDE"
  LD_LIBRARY_PATH="$GEOPMD_LIBDIR:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$GEOPMD_LIBDIR"
  AC_SUBST([GEOPMD_LIBDIR])
  AC_SUBST([GEOPMD_INCLUDE])
fi
AC_ARG_WITH([geopmd-include], [AS_HELP_STRING([--with-geopmd-include=PATH],
            [specify directory for the installed geopmd include files.])])
if test "x$with_geopmd_include" != x; then
  GEOPMD_INCLUDE=$with_geopmd_include
  AM_CPPFLAGS="$AM_CPPFLAGS -I$GEOPMD_INCLUDE"
  AC_SUBST([GEOPMD_INCLUDE])
fi
AC_ARG_WITH([geopmd-lib], [AS_HELP_STRING([--with-geopmd-lib=PATH],
            [specify directory for installed geopmd library])])
if test "x$with_geopmd_lib" != x; then
  GEOPMD_LIBDIR=$with_geopmd_lib
  LD_LIBRARY_PATH="$GEOPMD_LIBDIR:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$GEOPMD_LIBDIR"
  AC_SUBST([GEOPMD_LIBDIR])
fi

# This must stay below the geopm-local block above
AC_ARG_WITH([geopm], [AS_HELP_STRING([--with-geopm=PATH],
            [specify directory for installed geopm package.])])
if test "x$with_geopm" != x; then
  GEOPM_INCLUDE=$with_geopm/include
  GEOPM_LIBDIR=$with_geopm/lib
  AM_CPPFLAGS="$AM_CPPFLAGS -I$GEOPM_INCLUDE"
  LD_LIBRARY_PATH="$GEOPM_LIBDIR:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$GEOPM_LIBDIR"
  AC_SUBST([GEOPM_LIBDIR])
  AC_SUBST([GEOPM_INCLUDE])
fi
AC_ARG_WITH([geopm-include], [AS_HELP_STRING([--with-geopm-include=PATH],
            [specify directory for the installed geopm include files.])])
if test "x$with_geopm_include" != x; then
  GEOPM_INCLUDE=$with_geopm_include
  AM_CPPFLAGS="$AM_CPPFLAGS -I$GEOPM_INCLUDE"
  AC_SUBST([GEOPM_INCLUDE])
fi
AC_ARG_WITH([geopm-lib], [AS_HELP_STRING([--with-geopm-lib=PATH],
            [specify directory for installed geopm library])])
if test "x$with_geopm_lib" != x; then
  GEOPM_LIBDIR=$with_geopm_lib
  LD_LIBRARY_PATH="$GEOPM_LIBDIR:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$GEOPM_LIBDIR"
  AC_SUBST([GEOPM_LIBDIR])
fi

AC_ARG_WITH([libelf], [AS_HELP_STRING([--with-libelf=PATH],
            [specify directory for installed libelf package.])])
if test "x$with_libelf" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_libelf/include"
  LD_LIBRARY_PATH="$with_libelf/lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_libelf/lib"
fi
AC_ARG_WITH([libelf-include], [AS_HELP_STRING([--with-libelf-include=PATH],
            [specify directory for the installed libelf include files.])])
if test "x$with_libelf_include" != x; then
  AM_CPPFLAGS="$AM_CPPFLAGS -I$with_libelf_include"
fi
AC_ARG_WITH([libelf-lib], [AS_HELP_STRING([--with-libelf-lib=PATH],
            [specify directory for installed libelf library.])])
if test "x$with_libelf_lib" != x; then
  LD_LIBRARY_PATH="$with_libelf_lib:$LD_LIBRARY_PATH"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_libelf_lib"
fi

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
  AC_DEFINE([GEOPM_ENABLE_MPI], [], [MPI support enabled.])
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
R_OPENMP
AC_LANG_POP([C++])

if test "x$enable_openmp" = "xno" ; then
  enable_openmp="0"
else
  enable_openmp="1"
fi

AC_SUBST([enable_openmp])
AM_CONDITIONAL([ENABLE_OPENMP], [test "x$enable_openmp" = "x1"])

if test "x$enable_mpi" = "x1"; then
  AM_LDFLAGS="$AM_LDFLAGS $MPI_CLDFLAGS"
  AM_CFLAGS="$AM_CFLAGS $MPI_CFLAGS"
  AM_CXXFLAGS="$AM_CXXFLAGS $MPI_CFLAGS"
  AM_FCFLAGS="$AM_FCFLAGS $MPI_FCFLAGS"
  AM_CPPFLAGS="$AM_CPPFLAGS $MPI_CFLAGS"
fi


[MPI_LDFLAGS=$MPI_CLDFLAGS]
[MPI_LIBS=$MPI_CLIBS]

[MPI_FCFLAGS=$MPI_FFLAGS]
[MPI_FFLAGS=$MPI_F77FLAGS]
[MPI_FCLIBS=$MPI_FLIBS]
[MPI_FLIBS=$MPI_F77LIBS]

[MPI_FCFLAGS=`echo $MPI_FCFLAGS | sed 's|-lmpigi||'`]
[MPI_FFFLAGS=`echo $MPI_FFFLAGS | sed 's|-lmpigi||'`]

AC_SUBST([MPI_CFLAGS])
AC_SUBST([MPI_LDFLAGS])
AC_SUBST([MPI_LIBS])

AC_SUBST([MPI_FCFLAGS])
AC_SUBST([MPI_FFLAGS])
AC_SUBST([MPI_FCLIBS])
AC_SUBST([MPI_FLIBS])

[AM_CFLAGS="$AM_CFLAGS $OPENMP_CXXFLAGS -std=c99"]
[AM_CXXFLAGS="$AM_CXXFLAGS $OPENMP_CXXFLAGS -std=c++17"]
[AM_FFLAGS="$AM_FFLAGS $OPENMP_CXXFLAGS"]
[AM_FCFLAGS="$AM_FCFLAGS $OPENMP_CXXFLAGS -fPIC -fPIE"]
[AM_LDFLAGS="$AM_LDFLAGS $OPENMP_CXXFLAGS"]
[AM_CPPFLAGS="$AM_CPPFLAGS $MPI_CXXFLAGS"]

# LDFLAGS must be set properly for the below AC_CHECK_LIB calls
[LDFLAGS_SAVE="$LDFLAGS"]
[LDFLAGS="$AM_LDFLAGS"]

# FLAGS must be set properly for the below AC_CHECK_HEADER calls
[CFLAGS_SAVE="$CFLAGS"]
[CXXFLAGS_SAVE="$CXXFLAGS"]
[CPPFLAGS_SAVE="$CPPFLAGS"]
[FCFLAGS_SAVE="$FCFLAGS"]
[CFLAGS="$AM_CFLAGS"]
[CXXFLAGS="$AM_CXXFLAGS"]
[CPPFLAGS="$AM_CPPFLAGS"]
[FCFLAGS="$AM_FCFLAGS"]

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
AM_CONDITIONAL([HAVE_IFX], [test ! -z "$FC" && $FC --version | grep ifx > /dev/null])
AM_CONDITIONAL([HAVE_ICX], [test ! -z "$CC" && $CC --version | grep icx > /dev/null])
AM_CONDITIONAL([HAVE_ICPX], [test ! -z "$CXX" && $CXX --version | grep icpx > /dev/null])

if test ! -z "$CC" && $CC --version | grep icx > /dev/null; then
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
[FCFLAGS="$FCFLAGS_SAVE"]

[AM_CFLAGS="$AM_CFLAGS -Wall -Wextra -Werror -Wno-unused-parameter -Wformat -Wformat-security $NO_FASTMATH -fPIC -fPIE"]
[AM_CXXFLAGS="$AM_CXXFLAGS -Wall -Wextra -Werror -Wno-unused-parameter -Wformat -Wformat-security $NO_FASTMATH -fPIC -fPIE"]
[LDFLAGS_NOPIE="$AM_LDFLAGS -Wl,-z,relro,-z,now,-z,noexecstack"]
[AM_LDFLAGS="$LDFLAGS_NOPIE -pie"]
[AM_FFLAGS="$AM_FFLAGS $NO_FASTMATH -fPIC"]
if test ! -z "$FC" && $FC --version | grep "GNU Fortran" > /dev/null; then
[AM_FFLAGS="$AM_FFLAGS -fno-range-check"]
[AM_FCFLAGS="$AM_FCFLAGS -fno-range-check"]
fi

AC_SUBST([AM_CFLAGS])
AC_SUBST([FASTMATH])
AC_SUBST([NO_FASTMATH])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_FFLAGS])
AC_SUBST([AM_FCFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST([LDFLAGS_NOPIE])

if test "x$enable_beta" = "x1"; then
  ENABLE_BETA=True
  AC_DEFINE([GEOPM_ENABLE_BETA], [ ], [Enables beta feature implementations.])
else
  ENABLE_BETA=False
fi
AC_SUBST([ENABLE_BETA])

GEOPM_SOURCE_DIR=$(readlink -f $srcdir/..)
AC_DEFINE_UNQUOTED([GEOPM_SOURCE_DIR], ["$GEOPM_SOURCE_DIR"], [Root of the GEOPM source directory])

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
AC_MSG_RESULT([bashcompdir        : ${bashcompdir}])
AC_MSG_RESULT([])
AC_MSG_RESULT([debug              : ${enable_debug}])
AC_MSG_RESULT([mpi                : ${enable_mpi}])
AC_MSG_RESULT([openmp             : ${enable_openmp}])
AC_MSG_RESULT([fortran            : ${enable_fortran}])
AC_MSG_RESULT([ompt               : ${enable_ompt}])
AC_MSG_RESULT([beta               : ${enable_beta}])
AC_MSG_RESULT([===============================================================================])
