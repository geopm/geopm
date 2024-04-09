#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/test_omp_outer_loop.py

if ENABLE_OPENMP
if ENABLE_MPI
check_PROGRAMS += test/test_omp_outer_loop
test_test_omp_outer_loop_SOURCES = test/test_omp_outer_loop.cpp
test_test_omp_outer_loop_SOURCES += $(model_source_files)
test_test_omp_outer_loop_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
test_test_omp_outer_loop_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
test_test_omp_outer_loop_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += test/test_omp_outer_loop.cpp
endif
