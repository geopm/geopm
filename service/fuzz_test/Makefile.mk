#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

check_PROGRAMS += fuzz_tests/geopmhash_fuzz_test \
                  fuzz_tests/geopmhash_reg_test \
                  #end
fuzz_tests_geopmhash_fuzz_test_SOURCES = fuzz_tests/geopmhash_harness.cpp
fuzz_tests_geopmhash_reg_test_SOURCES = fuzz_tests/geopmhash_harness.cpp \
                                        fuzz_tests/StandaloneFuzzTargetMain.c \
                                        # end
fuzz_tests_geopmhash_fuzz_test_CXXFLAGS = $(AM_CXXFLAGS) -fsanitize=fuzzer -fno-inline
fuzz_tests_geopmhash_reg_test_CXXFLAGS = $(AM_CXXFLAGS) -fno-inline
fuzz_tests_geopmhash_fuzz_test_LDADD = libgeopmd.la
fuzz_tests_geopmhash_reg_test_LDADD = libgeopmd.la
