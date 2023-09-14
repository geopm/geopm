#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

TUTORIAL_BIN = tutorial_0 \
               tutorial_1 \
               tutorial_2 \
               tutorial_3 \
               tutorial_4 \
               tutorial_5 \
               # end

TUTORIAL_LIB = agent/libgeopmagent_example_agent.so.2.0.0 \
               iogroup/libgeopmiogroup_example_iogroup.so.2.0.0 \
               plugin_load/alice/libgeopmiogroup_alice.so.2.0.0 \
               plugin_load/bob/libgeopmiogroup_bob.so.2.0.0
               # end

GEOPM_PLUGIN_FLAGS=-fPIC -shared

CC ?= gcc
CXX ?= g++
MPICC ?= mpicc
MPICXX ?= mpicxx

all: $(TUTORIAL_BIN) $(TUTORIAL_LIB)

tutorial_0.o: tutorial_0.c
	$(MPICC) $(CFLAGS) -c $^ -o $@

tutorial_0: tutorial_0.o
	$(MPICC) $^ -o $@ $(LDFLAGS)

tutorial_1.o: tutorial_1.c
	$(MPICC) $(CFLAGS) -c $^ -o $@

tutorial_1: tutorial_1.o tutorial_region.o
	$(MPICC) $^ -o $@ $(LDFLAGS)

tutorial_2.o: tutorial_2.c
	$(MPICC) $(CFLAGS) -c $^ -o $@

tutorial_2: tutorial_2.o tutorial_region.o
	$(MPICC) $^ -o $@ $(LDFLAGS) -lgeopm

tutorial_3.o: tutorial_3.c
	$(MPICC) $(CFLAGS) -c $^ -o $@

tutorial_3: tutorial_3.o tutorial_region.o
	$(MPICC) $^ -o $@ $(LDFLAGS) -lgeopm

tutorial_4.o: tutorial_4.c
	$(MPICC) $(CFLAGS) -c $^ -o $@

tutorial_4: tutorial_4.o tutorial_region.o
	$(MPICC) $^ -o $@ $(LDFLAGS) -lgeopm -lstdc++

tutorial_5.o: tutorial_5.c
	$(MPICC) $(CFLAGS) -c $^ -o $@

tutorial_5: tutorial_5.o tutorial_region.o tutorial_region_prof.o
	$(MPICC) $^ -o $@ $(LDFLAGS) -lgeopm -lstdc++

tutorial_region.o: tutorial_region.c tutorial_region.h
	$(MPICC) $(CFLAGS) -c tutorial_region.c -o $@

tutorial_region_prof.o: tutorial_region_prof.c tutorial_region.h
	$(MPICC) $(CFLAGS) -c tutorial_region_prof.c -o $@

agent/libgeopmagent_example_agent.so.2.0.0: agent/ExampleAgent.cpp agent/ExampleAgent.hpp
	$(CXX) $(CXXFLAGS) -I ./agent $(GEOPM_PLUGIN_FLAGS) agent/ExampleAgent.cpp -o $@

iogroup/libgeopmiogroup_example_iogroup.so.2.0.0: iogroup/ExampleIOGroup.cpp iogroup/ExampleIOGroup.hpp
	$(CXX) $(CXXFLAGS) -I ./iogroup $(GEOPM_PLUGIN_FLAGS) iogroup/ExampleIOGroup.cpp -o $@

plugin_load/alice/libgeopmiogroup_alice.so.2.0.0: plugin_load/alice/AliceIOGroup.cpp plugin_load/alice/AliceIOGroup.hpp
	$(CXX) $(CXXFLAGS) -I ./plugin_load/alice $(GEOPM_PLUGIN_FLAGS) plugin_load/alice/AliceIOGroup.cpp -o $@

plugin_load/bob/libgeopmiogroup_bob.so.2.0.0: plugin_load/bob/BobIOGroup.cpp plugin_load/bob/BobIOGroup.hpp
	$(CXX) $(CXXFLAGS) -I ./plugin_load/bob $(GEOPM_PLUGIN_FLAGS) plugin_load/bob/BobIOGroup.cpp -o $@

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
