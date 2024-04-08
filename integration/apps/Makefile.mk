#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += apps/apps.py \
              apps/build_func.sh \
              apps/__init__.py \
              apps/README.md \
              # end

include apps/private.mk
include apps/amg/Makefile.mk
include apps/arithmetic_intensity/Makefile.mk
include apps/geopmbench/Makefile.mk
include apps/hpcg/Makefile.mk
include apps/hpl_mkl/Makefile.mk
include apps/hpl_netlib/Makefile.mk
include apps/minife/Makefile.mk
include apps/nekbone/Makefile.mk
include apps/nasft/Makefile.mk
include apps/parres/Makefile.mk
include apps/pennant/Makefile.mk
include apps/qe/Makefile.mk
