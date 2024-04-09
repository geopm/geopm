#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/test_frequency_hint_usage.py

if ENABLE_OPENMP
if ENABLE_MPI
check_PROGRAMS += test/test_frequency_hint_usage
test_test_frequency_hint_usage_SOURCES = test/test_frequency_hint_usage.cpp
test_test_frequency_hint_usage_SOURCES += $(model_source_files)
test_test_frequency_hint_usage_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
test_test_frequency_hint_usage_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
test_test_frequency_hint_usage_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += test/test_frequency_hint_usage.cpp
endif
