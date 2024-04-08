#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


EXTRA_DIST += apps/nasft/nasft.py \
              apps/nasft/__init__.py \
              apps/nasft/README.md \
              # end

include apps/nasft/nasft/Makefile.mk
