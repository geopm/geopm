#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


EXTRA_DIST += integration/apps/nasft/nasft.py \
              integration/apps/nasft/__init__.py \
              integration/apps/nasft/README.md \
              # end

include integration/apps/nasft/nasft/Makefile.mk
