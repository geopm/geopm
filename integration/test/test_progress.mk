#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/test_progress.py

if ENABLE_OPENMP
if ENABLE_MPI
noinst_PROGRAMS += test/test_progress
test_test_progress_SOURCES = test/test_progress.cpp
test_test_progress_SOURCES += $(model_source_files)
test_test_progress_LDADD = $(MATH_LIB) $(MPI_CLIBS)
test_test_progress_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
test_test_progress_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += test/test_progress.cpp
endif
