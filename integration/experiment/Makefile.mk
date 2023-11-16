#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/experiment/common_args.py \
              integration/experiment/__init__.py \
              integration/experiment/gen_pbs.sh \
              integration/experiment/gen_slurm.sh \
              integration/experiment/launch_util.py \
              integration/experiment/machine.py \
              integration/experiment/plotting.py \
              integration/experiment/README.md \
              integration/experiment/report.py \
              integration/experiment/run_experiment.py \
              integration/experiment/util.py \
              integration/experiment/outlier/README.md \
              integration/experiment/outlier/outlier_detection.py \
              integration/experiment/outlier/theta_nodelist_broken.txt \
              integration/experiment/system_characterization/host_config.py \
              # end

include integration/experiment/energy_efficiency/Makefile.mk
include integration/experiment/frequency_sweep/Makefile.mk
include integration/experiment/gpu_frequency_sweep/Makefile.mk
include integration/experiment/monitor/Makefile.mk
include integration/experiment/power_sweep/Makefile.mk
include integration/experiment/trace_analysis/Makefile.mk
include integration/experiment/uncore_frequency_sweep/Makefile.mk
