#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/apps/apps.py \
              integration/apps/build_func.sh \
              integration/apps/__init__.py \
              integration/apps/README.md \
              # end

include integration/apps/private.mk
include integration/apps/amg/Makefile.mk
include integration/apps/arithmetic_intensity/Makefile.mk
include integration/apps/geopmbench/Makefile.mk
include integration/apps/hpcg/Makefile.mk
include integration/apps/hpl_mkl/Makefile.mk
include integration/apps/hpl_netlib/Makefile.mk
include integration/apps/minife/Makefile.mk
include integration/apps/nekbone/Makefile.mk
include integration/apps/nasft/Makefile.mk
include integration/apps/parres/Makefile.mk
include integration/apps/pennant/Makefile.mk
include integration/apps/qe/Makefile.mk
