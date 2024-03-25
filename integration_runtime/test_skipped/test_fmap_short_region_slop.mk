#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test_skipped/test_fmap_short_region_slop.py \
              integration/test_skipped/test_fmap_short_region_slop.sbatch \
              # end

if ENABLE_OPENMP
if ENABLE_MPI
check_PROGRAMS += integration/test_skipped/test_fmap_short_region_slop
integration_test_skipped_test_fmap_short_region_slop_SOURCES = integration/test_skipped/test_fmap_short_region_slop.cpp
integration_test_skipped_test_fmap_short_region_slop_SOURCES += $(model_source_files)
integration_test_skipped_test_fmap_short_region_slop_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
integration_test_skipped_test_fmap_short_region_slop_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
integration_test_skipped_test_fmap_short_region_slop_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += integration/test_skipped/test_fmap_short_region_slop.cpp
endif
