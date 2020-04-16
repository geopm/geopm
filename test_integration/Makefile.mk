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

EXTRA_DIST += test_integration/configure_test_template.sh \
              test_integration/geopm_context.py \
              test_integration/geopm_test_integration.py \
              test_integration/geopm_test_launcher.py \
              test_integration/geopm_test_loop.sh \
              test_integration/__init__.py \
              test_integration/__main__.py \
              test_integration/README.md \
              test_integration/short_region/plot_margin_sweep.py \
              test_integration/test_profile_policy.py \
              test_integration/test_plugin_static_policy.py \
              test_integration/test_template.cpp.in \
              test_integration/test_template.mk.in \
              test_integration/test_template.py.in \
              test_integration/util.py \
              # end

if ENABLE_BETA
noinst_PROGRAMS += test_integration/test_profile_policy
test_integration_test_profile_policy_SOURCES = test_integration/test_profile_policy.cpp \
                                               # end
test_integration_test_profile_policy_LDADD = libgeopmpolicy.la
test_integration_test_profile_policy_LDFLAGS = $(AM_LDFLAGS)
test_integration_test_profile_policy_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11

endif

include test_integration/test_ee_short_region_slop.mk
include test_integration/test_ee_timed_scaling_mix.mk
include test_integration/test_enforce_policy.mk
include test_integration/test_omp_outer_loop.mk
include test_integration/test_scaling_region.mk
include test_integration/test_timed_scaling_region.mk
include test_integration/test_tutorial_base.mk
include test_integration/test_frequency_hint_usage.mk
