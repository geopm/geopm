#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

EXTRA_DIST += test_integration/geopm_context.py \
              test_integration/geopm_test_integration.py \
              test_integration/geopm_test_launcher.py \
              test_integration/geopm_test_loop.sh \
              test_integration/__init__.py \
              test_integration/__main__.py \
              test_integration/README.md \
              test_integration/test_ee_stream_dgemm_mix.py \
              test_integration/test_static_policy.py \
              test_integration/util.py \
              # end

if ENABLE_MPI
noinst_PROGRAMS += test_integration/test_ee_stream_dgemm_mix \
                   # end

test_integration_test_ee_stream_dgemm_mix_SOURCES = test_integration/test_ee_stream_dgemm_mix.cpp \
                                                    src/ModelRegion.cpp \
                                                    src/ModelRegion.hpp \
                                                    # end
test_integration_test_ee_stream_dgemm_mix_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
test_integration_test_ee_stream_dgemm_mix_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
test_integration_test_ee_stream_dgemm_mix_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif

noinst_PROGRAMS += test_integration/test_static_policy \
                   # end

test_integration_test_static_policy_SOURCES = test_integration/test_static_policy.c \
                                              # end
test_integration_test_static_policy_LDADD = libgeopmpolicy.la
test_integration_test_static_policy_LDFLAGS = $(AM_LDFLAGS)
test_integration_test_static_policy_CFLAGS = $(AM_CFLAGS) -D_GNU_SOURCE
