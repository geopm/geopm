#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

TUTORIAL_LIB = libgeopmagent_cpu_torch_agent.so.0.0.0 \
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

clean:
	rm -f $(TUTORIAL_LIB) *.o

.PHONY: all clean check
