#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_profile_policy.py

if ENABLE_BETA
noinst_PROGRAMS += integration/test/test_profile_policy
integration_test_test_profile_policy_SOURCES = integration/test/test_profile_policy.cpp \
                                               # end
integration_test_test_profile_policy_LDADD = libgeopm.la
integration_test_test_profile_policy_LDFLAGS = $(AM_LDFLAGS)
integration_test_test_profile_policy_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11
else
EXTRA_DIST += integration/test/test_profile_policy.cpp
endif
