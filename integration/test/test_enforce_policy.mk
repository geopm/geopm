#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_enforce_policy.py

noinst_PROGRAMS += integration/test/test_enforce_policy \
                   # end
integration_test_test_enforce_policy_SOURCES = integration/test/test_enforce_policy.c \
                                               # end
integration_test_test_enforce_policy_LDADD = libgeopm.la
integration_test_test_enforce_policy_LDFLAGS = $(AM_LDFLAGS)
integration_test_test_enforce_policy_CFLAGS = $(AM_CFLAGS)
