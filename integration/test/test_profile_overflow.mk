#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_profile_overflow.py

if ENABLE_OPENMP
if ENABLE_MPI
noinst_PROGRAMS += integration/test/test_profile_overflow
integration_test_test_profile_overflow_SOURCES = integration/test/test_profile_overflow.cpp
integration_test_test_profile_overflow_SOURCES += $(model_source_files)
integration_test_test_profile_overflow_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
integration_test_test_profile_overflow_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
integration_test_test_profile_overflow_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += integration/test/test_profile_overflow.cpp
endif
