#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/test_hint_time.py

if ENABLE_OPENMP
if ENABLE_MPI
noinst_PROGRAMS += test/test_hint_time
test_test_hint_time_SOURCES = test/test_hint_time.cpp
test_test_hint_time_SOURCES += $(model_source_files)
test_test_hint_time_LDADD = $(MATH_LIB) $(MPI_CLIBS)
test_test_hint_time_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
test_test_hint_time_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += test/test_hint_time.cpp
endif
