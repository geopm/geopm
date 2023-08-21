#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

check_PROGRAMS += fuzz_test/geopmhash_fuzz_test \
                  fuzz_test/geopmhash_reg_test \
                  #end
fuzz_test_geopmhash_fuzz_test_SOURCES = fuzz_test/geopmhash_harness.cpp
fuzz_test_geopmhash_reg_test_SOURCES = fuzz_test/geopmhash_harness.cpp \
                                       fuzz_test/StandaloneFuzzTargetMain.c \
                                       # end
fuzz_test_geopmhash_fuzz_test_CXXFLAGS = $(AM_CXXFLAGS) -fsanitize=fuzzer -fno-inline
fuzz_test_geopmhash_reg_test_CXXFLAGS = $(AM_CXXFLAGS) -fno-inline
fuzz_test_geopmhash_fuzz_test_LDADD = libgeopmd.la
fuzz_test_geopmhash_reg_test_LDADD = libgeopmd.la
