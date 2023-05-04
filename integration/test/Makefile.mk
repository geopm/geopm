#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/check_trace.py \
              integration/test/configure_test_template.sh \
              integration/test/geopm_test_launcher.py \
              integration/test/geopm_test_loop.sh \
              integration/test/__init__.py \
              integration/test/README.md \
              integration/test/short_region/plot_margin_sweep.py \
              integration/test/test_plugin_static_policy.py \
              integration/test/test_template.cpp.in \
              integration/test/test_template.mk.in \
              integration/test/test_template.py.in \
              integration/test/util.py \
              integration/test/test_tutorial_python_agents.py \
              integration/test/test_application_totals_pinning.py \
              integration/test/test_init_control.py \
              integration/test/test_time.py \
              # end

include integration/test/test_enforce_policy.mk
include integration/test/test_omp_outer_loop.mk
include integration/test/test_profile_policy.mk
include integration/test/test_scaling_region.mk
include integration/test/test_timed_scaling_region.mk
include integration/test/test_tutorial_base.mk
include integration/test/test_frequency_hint_usage.mk
include integration/test/test_power_balancer.mk
include integration/test/test_profile_overflow.mk
include integration/test/test_trace.mk
include integration/test/test_monitor.mk
include integration/test/test_ompt.mk
include integration/test/test_geopmio.mk
include integration/test/test_launch_application.mk
include integration/test/test_launch_pthread.mk
include integration/test/test_geopmagent.mk
include integration/test/test_environment.mk
include integration/test/test_power_governor.mk
include integration/test/test_environment.mk
include integration/test/test_frequency_map.mk
include integration/test/test_gpu_activity_agent.mk
include integration/test/test_hint_time.mk
include integration/test/test_progress.mk
include integration/test/test_programmable_counters.mk
include integration/test/test_cpu_activity_agent.mk
include integration/test/test_cpu_characterization.mk
include integration/test/test_multi_app.mk
