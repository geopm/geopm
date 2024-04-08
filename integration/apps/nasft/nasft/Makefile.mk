#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += apps/nasft/nasft/COPYING-NAS \
              apps/nasft/nasft/README \
              apps/nasft/nasft/make.def \
              # end

if ENABLE_FORTRAN
if ENABLE_MPI
if ENABLE_OPENMP
    check_PROGRAMS += apps/nasft/nasft/nas_ft
    apps_nasft_nasft_nas_ft_SOURCES = apps/nasft/nasft/ft.f90 \
                                  apps/nasft/nasft/global.fi \
                                  apps/nasft/nasft/mpinpb.fi \
                                  apps/nasft/nasft/npbparams.fi \
                                  apps/nasft/nasft/print_results.f \
                                  apps/nasft/nasft/randi8.f \
                                  apps/nasft/nasft/timers.f \
                                  # end

    apps_nasft_nasft_nas_ft_LDADD = libgeopm.la libgeopmfort.la $(MPI_FCLIBS) $(MPI_CXXLIBS)
    apps_nasft_nasft_nas_ft_LDFLAGS = $(LDFLAGS_NOPIE) $(MPI_LDFLAGS) $(OPENMP_CFLAGS)
    apps_nasft_nasft_nas_ft_FCFLAGS = -std=legacy -fiopenmp -msse4.2 $(MPI_FCFLAGS) $(OPENMP_CFLAGS) -O3
    apps_nasft_nasft_nas_ft_FFLAGS =  -fiopenmp -msse4.2 $(MPI_FFLAGS) $(OPENMP_CFLAGS) -O3
if HAVE_IFX
    apps_nasft_nasft_nas_ft_FCFLAGS += -std=legacy -xAVX -shared-intel -mcmodel=medium -fpic
    apps_nasft_nasft_nas_ft_FFLAGS += -xAVX -shared-intel -mcmodel=medium -fpic
endif
endif
endif
endif
