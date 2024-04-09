#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GLOBAL SETTINGS
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS += -D_POSIX_C_SOURCE=200809L

EXTRA_DIST = README.md \
             config/australis_env.sh \
             config/build_env.sh \
             config/build.sh \
             config/dudley_env.sh \
             config/endeavor_env.sh \
             config/gnu_env.sh \
             config/README.md \
             config/run_env.sh \
             config/smng_env.sh \
             config/theta_env.sh \
             requirements.txt \
              # end
noinst_PROGRAMS =

include apps/Makefile.mk
include experiment/Makefile.mk
include test/Makefile.mk
include test_skipped/Makefile.mk
include smoke/Makefile.mk
