#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_enforce_policy.py

check_PROGRAMS += integration/test/test_enforce_policy \
                  # end
test_test_enforce_policy_SOURCES = integration/test/test_enforce_policy.c \
                                               # end
test_test_enforce_policy_LDADD = libgeopm.la
test_test_enforce_policy_LDFLAGS = $(AM_LDFLAGS)
test_test_enforce_policy_CFLAGS = $(AM_CFLAGS)
