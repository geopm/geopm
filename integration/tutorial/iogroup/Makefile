GEOPM_INC:=$(HOME)/build/geopm/include
GEOPM_LIB:=$(HOME)/build/geopm/lib
CXXFLAGS=-g -Wall -MMD -fpic -I$(GEOPM_INC) -std=c++17
LDFLAGS=-shared -L$(GEOPM_LIB)
LDLIBS=-lgeopmd

libgeopmiogroup_example_iogroup.so.2.0.0: ExampleIOGroup.o
	$(LINK.cc) $^ $(LDLIBS) -o $@

-include *.d

clean:
	rm -f ./*.o ./*.d ./*.so*

check: libgeopmiogroup_example_iogroup.so.2.0.0
	rm -f /tmp/geopm_example_control.$$(id -u)
	GEOPM_PLUGIN_PATH=$$PWD geopmread | grep -q TMP_FILE_CONTROL; test $$? -ne 0 # File does not exist, but control is listed
	echo foobar > /tmp/geopm_example_control.$$(id -u)
	GEOPM_PLUGIN_PATH=$$PWD geopmread | grep -q TMP_FILE_CONTROL; test $$? -ne 0  # File has bad value, but control is listed
	echo 42 > /tmp/geopm_example_control.$$(id -u)
	GEOPM_PLUGIN_PATH=$$PWD geopmread | grep -q TMP_FILE_CONTROL # File has good value, but control is not listed
	GEOPM_PLUGIN_PATH=$$PWD geopmread TMP_FILE_CONTROL board 0 | grep -q 42 # File has good value, but it was not read properly
	for NN in NICE_TIME SYSTEM_TIME TMP_FILE_CONTROL USER_TIME; do \
	    GEOPM_PLUGIN_PATH=$$PWD geopmread $$NN board 0 && \
	    GEOPM_PLUGIN_PATH=$$PWD geopmread EXAMPLE::$$NN board 0; done
	rm -f /tmp/geopm_example_control.$$(id -u)

.PHONY: clean check
