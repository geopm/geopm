#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/test_enforce_policy.py

noinst_PROGRAMS += test/test_enforce_policy \
                  # end
test_test_enforce_policy_SOURCES = test/test_enforce_policy.c \
                                               # end
test_test_enforce_policy_LDADD = libgeopm.la
test_test_enforce_policy_LDFLAGS = $(AM_LDFLAGS)
test_test_enforce_policy_CFLAGS = $(AM_CFLAGS)
