#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

EXTRA_DIST += examples/README.md \
              examples/example_policy.json \
              examples/amg2013/0001-Adding-geopm-markup-to-AMG.patch \
              examples/amg2013/README \
              examples/comd/0001-Marked-up-CoMD-code-for-use-with-GEOPM.patch \
              examples/comd/README \
              examples/minife/0001-Optimized-MiniFE-code-for-Intel-hardware.patch \
              examples/minife/0002-Marked-up-MiniFE-code-for-use-with-GEOPM.patch \
              examples/minife/0001-Adding-geopm-markup-to-CORAL-version-of-miniFE.patch \
              examples/minife/README \
              examples/nekbone/0001-Optimized-nekbone-code-for-Intel-hardware.patch \
              examples/nekbone/0002-Marked-up-nekbone-code-for-use-with-GEOPM.patch \
              examples/nekbone/README \
              examples/qbox/README \
              examples/qbox/0001-Adding-geopm-markup-for-qbox.patch \
              examples/qbox/0002-Fixing-run-scripts-and-adding-run-scipt-for-qbox.patch \
              examples/hacc/README \
              #end

noinst_PROGRAMS += examples/geopm_print_error
examples_geopm_print_error_SOURCES = examples/geopm_print_error.c
examples_geopm_print_error_LDADD = libgeopmpolicy.la

noinst_PROGRAMS += examples/geopm_platform_supported
examples_geopm_platform_supported_SOURCES = examples/geopm_platform_supported.cpp
examples_geopm_platform_supported_LDADD = libgeopmpolicy.la

noinst_PROGRAMS += examples/geopmhash
examples_geopmhash_SOURCES = examples/geopmhash.c
examples_geopmhash_LDADD = libgeopmpolicy.la

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
                       examples/print_affinity \
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
    noinst_PROGRAMS += examples/threaded_step \
                       examples/simple_prof_c \
                       #end
    examples_simple_prof_c_SOURCES = examples/simple_prof_c.c
    examples_simple_prof_c_LDADD = libgeopm.la $(MPI_CXXLIBS)
    examples_simple_prof_c_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CPPFLAGS) $(OPENMP_CFLAGS)
    examples_simple_prof_c_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS) $(OPENMP_CFLAGS)
    examples_simple_prof_c_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS) $(OPENMP_CFLAGS)
    examples_simple_prof_c_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS) $(OPENMP_CFLAGS)
    examples_print_affinity_SOURCES = examples/print_affinity.cpp
    examples_print_affinity_LDADD  = $(MPI_CXXLIBS)
    examples_print_affinity_LDFLAGS  = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_print_affinity_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS) $(OPENMP_CFLAGS)
    examples_print_affinity_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS) $(OPENMP_CFLAGS)
if ENABLE_FORTRAN
    noinst_PROGRAMS += examples/simple_prof_f
    examples_simple_prof_f_SOURCES = examples/simple_prof_f.f90
    examples_simple_prof_f_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CPPFLAGS) $(OPENMP_CFLAGS)
    examples_simple_prof_f_LDADD = libgeopm.la libgeopmfort.la $(MPI_FCLIBS) $(MPI_CXXLIBS)
    examples_simple_prof_f_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS) $(OPENMP_CFLAGS)
    examples_simple_prof_f_FCFLAGS = $(AM_FCFLAGS) $(MPI_FCFLAGS) $(OPENMP_CFLAGS)
endif
    examples_threaded_step_SOURCES = examples/threaded_step_example.c
    examples_threaded_step_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CPPFLAGS) $(OPENMP_CFLAGS)
    examples_threaded_step_LDADD = libgeopm.la $(MPI_CXXLIBS)
    examples_threaded_step_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS) $(OPENMP_CFLAGS)
    examples_threaded_step_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS) $(OPENMP_CFLAGS)
endif
endif
endif

include examples/fft/Makefile.mk
