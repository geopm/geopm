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

EXTRA_DIST += integration/apps/nasft/nasft/COPYING-NAS \
              integration/apps/nasft/nasft/README \
              integration/apps/nasft/nasft/make.def \
              # end

if ENABLE_FORTRAN
if ENABLE_MPI
if ENABLE_OPENMP
    noinst_PROGRAMS += integration/apps/nasft/nasft_E/nas_ft_E
    integration_apps_nasft_nasft_E_nas_ft_E_SOURCES = integration/apps/nasft/nasft_E/ft.f90 \
                                                      integration/apps/nasft/nasft_E/global.fi \
                                                      integration/apps/nasft/nasft_E/mpinpb.fi \
                                                      integration/apps/nasft/nasft_E/npbparams.fi \
                                                      integration/apps/nasft/nasft/print_results.f \
                                                      integration/apps/nasft/nasft/randi8.f \
                                                      integration/apps/nasft/nasft/timers.f \
                                                      # end

    integration_apps_nasft_nasft_E_nas_ft_E_LDADD = libgeopm.la libgeopmfort.la $(MPI_FCLIBS) $(MPI_CXXLIBS)
    integration_apps_nasft_nasft_E_nas_ft_E_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS) $(OPENMP_CFLAGS)
    integration_apps_nasft_nasft_E_nas_ft_E_FCFLAGS = -std=legacy -fopenmp -msse4.2 $(MPI_FCFLAGS) $(OPENMP_CFLAGS) -O3
    integration_apps_nasft_nasft_E_nas_ft_E_FFLAGS =  -fopenmp -msse4.2 $(MPI_FFLAGS) $(OPENMP_CFLAGS) -O3
if HAVE_IFORT
    integration_apps_nasft_nasft_E_nas_ft_E_FCFLAGS += -std=legacy -xAVX -shared-intel -mcmodel=medium -fpic
    integration_apps_nasft_nasft_E_nas_ft_E_FFLAGS += -xAVX -shared-intel -mcmodel=medium -fpic
endif
endif
endif
endif
