#
#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

TUTORIAL_BIN = tutorial_0 \
               tutorial_1 \
               tutorial_2 \
               tutorial_3 \
               tutorial_4 \
               tutorial_5 \
               # end

TUTORIAL_LIB = agent/libgeopmagent_example_agent.so.0.0.0 \
               iogroup/libgeopmiogroup_example_iogroup.so.0.0.0 \
               plugin_load/alice/libgeopmiogroup_alice.so.0.0.0 \
               plugin_load/bob/libgeopmiogroup_bob.so.0.0.0
               # end

GEOPM_PLUGIN_FLAGS=-fPIC -shared

CC ?= gcc
CXX ?= g++
MPICC ?= mpicc
MPICXX ?= mpicxx

all: $(TUTORIAL_BIN) $(TUTORIAL_LIB)

tutorial_0.o: tutorial_0.c
	$(MPICC) -c $^ $(CFLAGS) -o $@

tutorial_0: tutorial_0.o
	$(MPICC) $^ $(LDFLAGS) -o $@

tutorial_1.o: tutorial_1.c
	$(MPICC) -c $^ $(CFLAGS) -o $@

tutorial_1: tutorial_1.o tutorial_region.o
	$(MPICC) $^ $(LDFLAGS) -o $@

tutorial_2.o: tutorial_2.c
	$(MPICC) -c $^ $(CFLAGS) -o $@

tutorial_2: tutorial_2.o tutorial_region.o
	$(MPICC) $^ $(LDFLAGS) -lgeopm -o $@

tutorial_3.o: tutorial_3.c
	$(MPICC) -c $^ $(CFLAGS) -o $@

tutorial_3: tutorial_3.o tutorial_region.o
	$(MPICC) $^ $(LDFLAGS) -lgeopm -o $@

tutorial_4.o: tutorial_4.c
	$(MPICC) -c $^ $(CFLAGS) -o $@

tutorial_4: tutorial_4.o tutorial_region.o
	$(MPICC) $^ $(LDFLAGS) -lgeopm -lstdc++ -o $@

tutorial_5.o: tutorial_5.c
	$(MPICC) -c $^ $(CFLAGS) -o $@

tutorial_5: tutorial_5.o tutorial_region.o
	$(MPICC) $^ $(LDFLAGS) -lgeopm -lstdc++ -o $@

tutorial_region.o: tutorial_region.c tutorial_region.h
	$(MPICC) -c tutorial_region.c $(CFLAGS) -o $@

agent/libgeopmagent_example_agent.so.0.0.0: agent/ExampleAgent.cpp agent/ExampleAgent.hpp
	$(CXX) agent/ExampleAgent.cpp $(CXXFLAGS) -I ./agent $(GEOPM_PLUGIN_FLAGS) -o $@

iogroup/libgeopmiogroup_example_iogroup.so.0.0.0: iogroup/ExampleIOGroup.cpp iogroup/ExampleIOGroup.hpp
	$(CXX) iogroup/ExampleIOGroup.cpp $(CXXFLAGS) -I ./iogroup $(GEOPM_PLUGIN_FLAGS) -o $@

plugin_load/alice/libgeopmiogroup_alice.so.0.0.0: plugin_load/alice/AliceIOGroup.cpp plugin_load/alice/AliceIOGroup.hpp
	$(CXX) plugin_load/alice/AliceIOGroup.cpp $(CXXFLAGS) -I ./plugin_load/alice $(GEOPM_PLUGIN_FLAGS) -o $@

plugin_load/bob/libgeopmiogroup_bob.so.0.0.0: plugin_load/bob/BobIOGroup.cpp plugin_load/bob/BobIOGroup.hpp
	$(CXX) plugin_load/bob/BobIOGroup.cpp $(CXXFLAGS) -I ./plugin_load/bob $(GEOPM_PLUGIN_FLAGS) -o $@

clean:
	rm -f $(TUTORIAL_BIN) $(TUTORIAL_LIB) *.o

check: all
	./tutorial_0.sh
	./tutorial_1.sh
	./tutorial_2.sh
	./tutorial_3.sh
	./tutorial_4.sh
	./tutorial_5.sh
	./tutorial_6.sh

.PHONY: all clean check
