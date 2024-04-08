#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/check_trace.py \
              test/configure_test_template.sh \
              test/geopm_test_launcher.py \
              test/__init__.py \
              test/README.md \
              test/short_region/plot_margin_sweep.py \
              test/test_plugin_static_policy.py \
              test/test_template.cpp.in \
              test/test_template.mk.in \
              test/test_template.py.in \
              test/util.py \
              test/test_application_totals_pinning.py \
              test/test_init_control.py \
              test/test_time.py \
              test/plan/README.md \
              # end

include test/test_enforce_policy.mk
include test/test_omp_outer_loop.mk
include test/test_profile_policy.mk
include test/test_scaling_region.mk
include test/test_timed_scaling_region.mk
include test/test_tutorial_base.mk
include test/test_frequency_hint_usage.mk
include test/test_power_balancer.mk
include test/test_profile_overflow.mk
include test/test_trace.mk
include test/test_monitor.mk
include test/test_ompt.mk
include test/test_geopmio.mk
include test/test_launch_application.mk
include test/test_launch_pthread.mk
include test/test_geopmagent.mk
include test/test_environment.mk
include test/test_power_governor.mk
include test/test_environment.mk
include test/test_frequency_map.mk
include test/test_gpu_activity_agent.mk
include test/test_hint_time.mk
include test/test_progress.mk
include test/test_programmable_counters.mk
include test/test_cpu_activity_agent.mk
include test/test_cpu_characterization.mk
include test/test_multi_app.mk
include test/test_epoch_inference.mk
include test/test_gen_pbs.mk
