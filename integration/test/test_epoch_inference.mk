#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += test/test_epoch_inference.py

if ENABLE_OPENMP
if ENABLE_MPI
noinst_PROGRAMS += test/test_epoch_inference
test_test_epoch_inference_SOURCES = test/test_epoch_inference.cpp
test_test_epoch_inference_SOURCES += $(model_source_files)
test_test_epoch_inference_LDADD = $(MATH_LIB) $(MPI_CLIBS)
test_test_epoch_inference_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
test_test_epoch_inference_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
endif
else
EXTRA_DIST += test/test_epoch_inference.cpp
endif
