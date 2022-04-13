#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


EXTRA_DIST += integration/README.md \
              integration/config/build_env.sh \
              integration/config/build.sh \
              integration/config/dudley_env.sh \
              integration/config/endeavor_env.sh \
              integration/config/gnu_env.sh \
              integration/config/README.md \
              integration/config/run_env.sh \
              integration/config/smng_env.sh \
              integration/config/theta_env.sh \
              # end

include integration/apps/Makefile.mk
include integration/experiment/Makefile.mk
include integration/test/Makefile.mk
include integration/test_skipped/Makefile.mk
include integration/smoke/Makefile.mk
