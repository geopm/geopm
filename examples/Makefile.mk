#  Copyright (c) 2015, 2016, Intel Corporation
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

noinst_PROGRAMS += examples/geopm_print_error
examples_geopm_print_error_SOURCES = examples/geopm_print_error.c
examples_geopm_print_error_LDADD = libgeopmpolicy.la

if ENABLE_MPI
    noinst_PROGRAMS += examples/geopm_ctl_single \
                       examples/timed_region \
                       examples/simple_prof_c \
                       # end
    examples_geopm_ctl_single_SOURCES = examples/geopm_ctl_single.cpp
    examples_geopm_ctl_single_LDADD = libgeopm.la
    examples_geopm_ctl_single_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    examples_geopm_ctl_single_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_geopm_ctl_single_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_geopm_ctl_single_CXXFLAGS = $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
    examples_timed_region_SOURCES = examples/timed_region.cpp
    examples_timed_region_LDADD = libgeopm.la
    examples_timed_region_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    examples_timed_region_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_timed_region_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_timed_region_CXXFLAGS = $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
    examples_simple_prof_c_SOURCES = examples/simple_prof_c.c
    examples_simple_prof_c_LDADD = libgeopm.la
    examples_simple_prof_c_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    examples_simple_prof_c_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_simple_prof_c_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_simple_prof_c_CXXFLAGS = $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
if ENABLE_FORTRAN
    noinst_PROGRAMS += examples/simple_prof_f
    examples_simple_prof_f_SOURCES = examples/simple_prof_f.f90
    examples_simple_prof_f_LDADD = libgeopm.la libgeopmfort.la
    examples_simple_prof_f_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    examples_simple_prof_f_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS) -lmpifort
    examples_simple_prof_f_FCFLAGS = $(FCFLAGS) $(AM_FCFLAGS) $(MPI_CFLAGS)
endif
endif

if ENABLE_MPI
if ENABLE_SCHED
    noinst_PROGRAMS += examples/synthetic_benchmark
    examples_synthetic_benchmark_SOURCES = examples/synthetic_benchmark.cpp examples/synthetic_benchmark.hpp
    examples_synthetic_benchmark_LDADD = libgeopm.la
    examples_synthetic_benchmark_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    examples_synthetic_benchmark_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_synthetic_benchmark_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_synthetic_benchmark_CXXFLAGS = $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
endif
endif

if ENABLE_MPI
if ENABLE_OPENMP
if ENABLE_SCHED
    noinst_PROGRAMS += examples/threaded_step
    examples_threaded_step_SOURCES = examples/threaded_step_example.c
    examples_threaded_step_LDADD = libgeopm.la
    examples_threaded_step_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    examples_threaded_step_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    examples_threaded_step_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    examples_threaded_step_CXXFLAGS = $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
endif
endif
endif
