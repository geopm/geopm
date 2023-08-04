#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/test/test_ompt_gpu.py

if ENABLE_OPENMP
if ENABLE_MPI
noinst_PROGRAMS += integration/test/test_ompt_gpu
integration_test_test_ompt_gpu_SOURCES = integration/test/test_ompt_gpu.cpp
integration_test_test_ompt_gpu_SOURCES += $(model_source_files)
integration_test_test_ompt_gpu_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
integration_test_test_ompt_gpu_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
#TODO if NVIDIA use -fopenmp=libopm -fopenmp-targets=nvptx64-nvidia-cuda
integration_test_test_ompt_gpu_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS) -fiopenmp -fopenmp-targets=spir64
endif
else
EXTRA_DIST += integration/test/test_ompt_gpu.cpp
endif
