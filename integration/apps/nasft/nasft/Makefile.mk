#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/apps/nasft/nasft/COPYING-NAS \
              integration/apps/nasft/nasft/README \
              integration/apps/nasft/nasft/make.def \
              # end

if ENABLE_FORTRAN
if ENABLE_MPI
if ENABLE_OPENMP
    check_PROGRAMS += integration/apps/nasft/nasft/nas_ft
    integration_apps_nasft_nasft_nas_ft_SOURCES = integration/apps/nasft/nasft/ft.f90 \
                                  integration/apps/nasft/nasft/global.fi \
                                  integration/apps/nasft/nasft/mpinpb.fi \
                                  integration/apps/nasft/nasft/npbparams.fi \
                                  integration/apps/nasft/nasft/print_results.f \
                                  integration/apps/nasft/nasft/randi8.f \
                                  integration/apps/nasft/nasft/timers.f \
                                  # end

    integration_apps_nasft_nasft_nas_ft_LDADD = libgeopm.la libgeopmfort.la $(MPI_FCLIBS) $(MPI_CXXLIBS)
    integration_apps_nasft_nasft_nas_ft_LDFLAGS = $(LDFLAGS_NOPIE) $(MPI_LDFLAGS) $(OPENMP_CFLAGS)
    integration_apps_nasft_nasft_nas_ft_FCFLAGS = -std=legacy -fiopenmp -msse4.2 $(MPI_FCFLAGS) $(OPENMP_CFLAGS) -O3
    integration_apps_nasft_nasft_nas_ft_FFLAGS =  -fiopenmp -msse4.2 $(MPI_FFLAGS) $(OPENMP_CFLAGS) -O3
if HAVE_IFX
    integration_apps_nasft_nasft_nas_ft_FCFLAGS += -std=legacy -xAVX -shared-intel -mcmodel=medium -fpic
    integration_apps_nasft_nasft_nas_ft_FFLAGS += -xAVX -shared-intel -mcmodel=medium -fpic
endif
endif
endif
endif
