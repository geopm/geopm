#!/bin/bash
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

MKL_DIRS = $(MKLROOT)/lib/intel64
MKL_LIBS        = -Wl,--start-group $(MKL_DIRS)/libmkl_intel_lp64.a $(MKL_DIRS)/libmkl_intel_thread.a $(MKL_DIRS)/libmkl_core.a  -Wl,--end-group -liomp5 -lpthread -lm -ldl

ifeq ($(DEBUG), 1)
	DBG_FLAGS = -O0 -g
else
	DBG_FLAGS = -O3
endif

CFLAGS_EXTRA = -std=c++11 -D_POSIX_C_SOURCE=200809L $(DBG_FLAGS) -qopenmp -lrt -lpthread

all: dgemm freq_change_delay_test_simple

dgemm: dgemm.cpp
	icpc $(CFLAGS_EXTRA) $(MKL_LIBS) -mkl=parallel $(CFLAGS) $(LDFLAGS) $(LIBS) -qopenmp $^ -o $@

freq_change_delay_test_simple:  MSRIO.cpp freq_change_delay_test_simple.cpp
	icpc $^ $(CFLAGS_EXTRA) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

check: dgemm freq_change_delay_test_simple
	KMP_AFFINITY=granularity=fine,scatter OMP_NUM_THREADS=44 numactl --physcpubind=0-43 ./dgemm > dgemm.out & sleep 1; \
	KMP_AFFINITY=granularity=fine,scatter OMP_NUM_THREADS=44 numactl --physcpubind=44-87 ./freq_change_delay_test_simple 1.1e9 1.3e9; wait;

clean:
	rm -f dgemm freq_change_delay_test_simple

.PHONY: clean test
