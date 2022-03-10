#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

TUTORIAL_LIB = libgeopmagent_cpu_torch_agent.so.0.0.0 \
               libgeopmagent_gpu_torch_agent.so.0.0.0 \
               # end

GEOPM_PLUGIN_FLAGS=-fPIC -shared

#CC ?= gcc
#CXX ?= g++
#MPICC ?= mpicc
#MPICXX ?= mpicxx

all: $(TUTORIAL_LIB)

GEOPM_BIN=$(GEOPM_INSTALL)/bin
GEOPM_LIB=$(GEOPM_INSTALL)/lib
GEOPM_INC=$(GEOPM_INSTALL)/include
GEOPM_LDFLAGS=-L$(GEOPM_LIB)
GEOPM_CFLAGS=-I$(GEOPM_INC)

LIBTORCH_CPPFLAGS=-I${TORCH_ROOT}/include -I${TORCH_ROOT}/include/torch/csrc/api/include -std=c++14 -L${TORCH_ROOT}/lib -ltorch -ltorch_cpu -lc10 -ltorch_global_deps
EXTRA_CPPFLAGS:=$(LIBTORCH_CPPFLAGS) $(EXTRA_CPPFLAGS)

CXXFLAGS=$(GEOPM_CFLAGS) $(OMP_FLAGS)
LDFLAGS=$(GEOPM_LDFLAGS) $(OMP_FLAGS)

libgeopmagent_cpu_torch_agent.so.0.0.0: CPUTorchAgent.cpp CPUTorchAgent.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(GEOPM_PLUGIN_FLAGS) $(EXTRA_CPPFLAGS) CPUTorchAgent.cpp -o $@
libgeopmagent_gpu_torch_agent.so.0.0.0: GPUTorchAgent.cpp GPUTorchAgent.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(GEOPM_PLUGIN_FLAGS) $(EXTRA_CPPFLAGS) GPUTorchAgent.cpp -o $@

clean:
	rm -f $(TUTORIAL_LIB) *.o

.PHONY: all clean check
