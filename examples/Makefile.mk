#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += examples/README.md \
              examples/comd/0001-Marked-up-CoMD-code-for-use-with-GEOPM.patch \
              examples/comd/README \
              examples/custom_msr/msr_reasons.json \
              examples/hacc/README \
              examples/qbox/0001-Adding-geopm-markup-for-qbox.patch \
              examples/qbox/0002-Fixing-run-scripts-and-adding-run-scipt-for-qbox.patch \
              examples/qbox/README \
              examples/simple_pio_example.c \
              #end

noinst_PROGRAMS += examples/geopm_print_error
examples_geopm_print_error_SOURCES = examples/geopm_print_error.c
examples_geopm_print_error_LDADD = libgeopm.la

noinst_PROGRAMS += examples/geopm_platform_supported
examples_geopm_platform_supported_SOURCES = examples/geopm_platform_supported.cpp
examples_geopm_platform_supported_LDADD = libgeopm.la

noinst_PROGRAMS += examples/geopmhash
examples_geopmhash_SOURCES = examples/geopmhash.c
examples_geopmhash_LDADD = libgeopm.la

if ENABLE_MPI
    noinst_PROGRAMS += examples/timed_region
    examples_timed_region_SOURCES = examples/timed_region.cpp
    examples_timed_region_LDADD = libgeopm.la $(MPI_CXXLIBS)
    examples_timed_region_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_timed_region_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_timed_region_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
endif

if ENABLE_MPI
if ENABLE_SCHED
    noinst_PROGRAMS += examples/synthetic_benchmark \
                       # end
    examples_synthetic_benchmark_SOURCES = examples/synthetic_benchmark.cpp examples/synthetic_benchmark.hpp
    examples_synthetic_benchmark_LDADD = libgeopm.la $(MPI_CXXLIBS)
    examples_synthetic_benchmark_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_synthetic_benchmark_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_synthetic_benchmark_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
endif
endif

if ENABLE_MPI
if ENABLE_OPENMP
if ENABLE_SCHED
    noinst_PROGRAMS += examples/simple_prof_c \
                       examples/print_affinity \
                       #end
    examples_simple_prof_c_SOURCES = examples/simple_prof_c.c
    examples_simple_prof_c_LDADD = libgeopm.la $(MPI_CXXLIBS)
    examples_simple_prof_c_CPPFLAGS = $(AM_CPPFLAGS)
    examples_simple_prof_c_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS)
    examples_simple_prof_c_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_simple_prof_c_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
    examples_print_affinity_SOURCES = examples/print_affinity.cpp
    examples_print_affinity_LDADD  = $(MPI_CXXLIBS)
    examples_print_affinity_LDFLAGS  = $(AM_LDFLAGS) $(MPI_LDFLAGS)
    examples_print_affinity_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_print_affinity_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
if ENABLE_FORTRAN
    noinst_PROGRAMS += examples/simple_prof_f
    examples_simple_prof_f_SOURCES = examples/simple_prof_f.f90
    examples_simple_prof_f_CPPFLAGS = $(AM_CPPFLAGS)
    examples_simple_prof_f_LDADD = libgeopm.la libgeopmfort.la $(MPI_FCLIBS) $(MPI_CXXLIBS)
    examples_simple_prof_f_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS)
    examples_simple_prof_f_FCFLAGS = $(AM_FCFLAGS) $(MPI_FCFLAGS)
endif
endif
endif
endif
