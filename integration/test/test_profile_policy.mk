#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_profile_policy.py

if ENABLE_BETA
check_PROGRAMS += integration/test/test_profile_policy
test_test_profile_policy_SOURCES = integration/test/test_profile_policy.cpp \
                                               # end
test_test_profile_policy_LDADD = libgeopm.la
test_test_profile_policy_LDFLAGS = $(AM_LDFLAGS)
test_test_profile_policy_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11
else
EXTRA_DIST += integration/test/test_profile_policy.cpp
endif
