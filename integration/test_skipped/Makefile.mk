#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test_skipped/test_reporter.py \
              integration/test_skipped/test_reporter.cpp \
              # end

include integration/test_skipped/test_fmap_short_region_slop.mk
include integration/test_skipped/test_levelzero_signals.mk
