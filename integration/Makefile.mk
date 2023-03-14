#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


EXTRA_DIST += integration/README.md \
              integration/config/australis_env.sh \
              integration/config/build_env.sh \
              integration/config/build.sh \
              integration/config/dudley_env.sh \
              integration/config/endeavor_env.sh \
              integration/config/gnu_env.sh \
              integration/config/README.md \
              integration/config/run_env.sh \
              integration/config/smng_env.sh \
              integration/config/theta_env.sh \
              integration/requirements.txt \
              integration/test-no-mpi/example_output/test_sleep_region_map_report.yaml \
              integration/test-no-mpi/example_output/test_sleep_region_map_trace_profile.csv \
              integration/test-no-mpi/example_output/test_sleep_region_report.yaml \
              integration/test-no-mpi/example_output/test_sleep_region_trace_profile.csv \
              integration/test-no-mpi/example_output/test_sleep_report.yaml \
              integration/test-no-mpi/example_output/test_sleep_trace_profile.csv \
              integration/test-no-mpi/README.md \
              integration/test-no-mpi/Makefile \
              integration/test-no-mpi/test_sleep_region.c \
              integration/test-no-mpi/test_sleep_region_map.sh \
              integration/test-no-mpi/test_sleep_region.sh \
              integration/test-no-mpi/test_sleep.sh \
              # end

include integration/apps/Makefile.mk
include integration/experiment/Makefile.mk
include integration/test/Makefile.mk
include integration/test_skipped/Makefile.mk
include integration/smoke/Makefile.mk
