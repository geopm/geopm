#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += experiment/common_args.py \
              experiment/__init__.py \
              experiment/gen_pbs.sh \
              experiment/gen_slurm.sh \
              experiment/launch_util.py \
              experiment/machine.py \
              experiment/plotting.py \
              experiment/README.md \
              experiment/report.py \
              experiment/run_experiment.py \
              experiment/util.py \
              experiment/outlier/README.md \
              experiment/outlier/outlier_detection.py \
              experiment/outlier/theta_nodelist_broken.txt \
              experiment/system_characterization/host_config.py \
              experiment/system_characterization/test_host_config.py \
              # end

include experiment/energy_efficiency/Makefile.mk
include experiment/frequency_sweep/Makefile.mk
include experiment/gpu_frequency_sweep/Makefile.mk
include experiment/monitor/Makefile.mk
include experiment/power_sweep/Makefile.mk
include experiment/trace_analysis/Makefile.mk
include experiment/uncore_frequency_sweep/Makefile.mk
