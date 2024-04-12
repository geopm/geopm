GEOPM_INC:=$(HOME)/build/geopm/include
GEOPM_LIB:=$(HOME)/build/geopm/lib
CXXFLAGS=-g -Wall -MMD -fpic -I$(GEOPM_INC) -std=c++17
LDFLAGS=-shared -L$(GEOPM_LIB)
LDLIBS=-lgeopmd

libgeopmagent_example_agent.so.2.0.0: ExampleAgent.o
	$(LINK.cc) $^ $(LDLIBS) -o $@

-include *.d

clean:
	rm -f ./*.o ./*.d ./*.so*

.PHONY: clean
