#  Copyright (c) 2015 - 2017, Intel Corporation
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
if ENABLE_MPI
    check_PROGRAMS += test_integration/geopm_test_integration
    test_integration_geopm_test_integration_SOURCES = tutorial/ModelRegion.hpp \
                                                      tutorial/ModelRegion.cpp \
                                                      tutorial/ModelApplication.hpp \
                                                      tutorial/ModelApplication.cpp \
                                                      tutorial/imbalancer.h \
                                                      tutorial/Imbalancer.cpp \
                                                      tutorial/tutorial_6.cpp \
                                                      # end

    test_integration_geopm_test_integration_LDADD = libgeopm.la $(MPI_CLIBS)
    test_integration_geopm_test_integration_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS)
    test_integration_geopm_test_integration_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    test_integration_geopm_test_integration_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS)
endif

EXTRA_DIST += test_integration/geopm_test_integration.py \
              test_integration/geopm_io.py \
              test_integration/geopm_launcher.py \
              test_integration/geopm_test_loop.sh \
              test_integration/README.md \
              # end
