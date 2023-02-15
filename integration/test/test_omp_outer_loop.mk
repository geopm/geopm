#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_omp_outer_loop.py

if ENABLE_OPENMP
if ENABLE_MPI
noinst_PROGRAMS += integration/test/test_omp_outer_loop
integration_test_test_omp_outer_loop_SOURCES = integration/test/test_omp_outer_loop.cpp
integration_test_test_omp_outer_loop_SOURCES += $(model_source_files)
integration_test_test_omp_outer_loop_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
integration_test_test_omp_outer_loop_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
integration_test_test_omp_outer_loop_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += integration/test/test_omp_outer_loop.cpp
endif
