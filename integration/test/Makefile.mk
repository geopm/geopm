#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

EXTRA_DIST += integration/test/check_trace.py \
              integration/test/configure_test_template.sh \
              integration/test/geopm_context.py \
              integration/test/geopm_test_integration.py \
              integration/test/geopm_test_launcher.py \
              integration/test/geopm_test_loop.sh \
              integration/test/__init__.py \
              integration/test/__main__.py \
              integration/test/README.md \
              integration/test/short_region/plot_margin_sweep.py \
              integration/test/test_plugin_static_policy.py \
              integration/test/test_template.cpp.in \
              integration/test/test_template.mk.in \
              integration/test/test_template.py.in \
              integration/test/util.py \
              # end

include integration/test/test_ee_short_region_slop.mk
include integration/test/test_ee_timed_scaling_mix.mk
include integration/test/test_enforce_policy.mk
include integration/test/test_omp_outer_loop.mk
include integration/test/test_profile_policy.mk
include integration/test/test_scaling_region.mk
include integration/test/test_timed_scaling_region.mk
include integration/test/test_tutorial_base.mk
include integration/test/test_frequency_hint_usage.mk
include integration/test/test_epoch_inference.mk
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
