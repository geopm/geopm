#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

TUTORIAL_LIB = libgeopmagent_cpu_ffnet_agent.so.0.0.0 \
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

EXTRA_CPPFLAGS:=$(EXTRA_CPPFLAGS)

CXXFLAGS=$(GEOPM_CFLAGS) $(OMP_FLAGS)
LDFLAGS=$(GEOPM_LDFLAGS) $(OMP_FLAGS)

libgeopmagent_cpu_ffnet_agent.so.0.0.0: CPUFFNetAgent.cpp CPUFFNetAgent.hpp LocalNeuralNet.o TensorOneD.o TensorTwoD.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(GEOPM_PLUGIN_FLAGS) $(EXTRA_CPPFLAGS) CPUFFNetAgent.cpp LocalNeuralNet.o TensorOneD.o TensorTwoD.o -o $@

TensorOneD.o: TensorOneD.cpp TensorOneD.hpp
	$(CXX) $(CXXFLAGS) $(GEOPM_PLUGIN_FLAGS) -c TensorOneD.cpp -o $@

TensorTwoD.o: TensorTwoD.cpp TensorTwoD.hpp TensorOneD.hpp
	$(CXX) $(CXXFLAGS) $(GEOPM_PLUGIN_FLAGS) -c TensorTwoD.cpp -o $@

LocalNeuralNet.o: LocalNeuralNet.cpp LocalNeuralNet.hpp TensorOneD.hpp TensorTwoD.hpp
	$(CXX) $(CXXFLAGS) $(GEOPM_PLUGIN_FLAGS) -c LocalNeuralNet.cpp -o $@

clean:
	rm -f $(TUTORIAL_LIB) *.o 

.PHONY: all clean check
