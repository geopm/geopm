#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GLOBAL SETTINGS
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS += -I$(top_srcdir)/src \
               -DGEOPM_DEFAULT_PLUGIN_PATH=\"$(libdir)/geopm\" \
               -D_POSIX_C_SOURCE=200809L

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmd.la

bin_PROGRAMS = geopmread \
               geopmwrite \
               # end

include_HEADERS = src/geopm_debug.hpp \
                  src/geopm_error.h \
                  src/geopm_field.h \
                  src/geopm_hash.h \
                  src/geopm_hint.h \
                  src/geopm_pio.h \
                  src/geopm_plugin.hpp \
                  src/geopm_sched.h \
                  src/geopm_time.h \
                  src/geopm_topo.h \
                  src/geopm_version.h \
                  # end

geopmincludedir = $(includedir)/geopm

geopminclude_HEADERS = contrib/json11/json11.hpp \
                       src/geopm/Agg.hpp \
                       src/geopm/CircularBuffer.hpp \
                       src/geopm/Exception.hpp \
                       src/geopm/Helper.hpp \
                       src/geopm/IOGroup.hpp \
                       src/geopm/MSRIOGroup.hpp \
                       src/geopm/PlatformIO.hpp \
                       src/geopm/PlatformTopo.hpp \
                       src/geopm/PluginFactory.hpp \
                       src/geopm/SharedMemory.hpp \
                       src/geopm/SharedMemoryScopedLock.hpp \
                       # end

install-exec-hook: install-python install-plugin-dir
dist: dist-googletest base_man

install-plugin-dir:
	$(INSTALL) -d $(DESTDIR)/$(pkglibdir)

dist_doc_DATA = COPYING \
                COPYING-TPP \
                README.rst \
                VERSION \
                # end

EXTRA_DIST = .gitignore \
             Makefile.am \
             autogen.sh \
             configure.ac \
             contrib/json11/LICENSE.txt \
             contrib/json11/README.md \
             dox/blurb.md \
             dox/Doxyfile.in \
             geopm-service.spec.in \
             io.github.geopm.conf \
             io.github.geopm.xml \
             geopmaccess \
             geopmd \
             geopmdpy/__init__.py \
             geopmdpy/__main__.py \
             geopmdpy/access.py \
             geopmdpy/dbus_xml.py \
             geopmdpy/error.py \
             geopmdpy/gffi.py \
             geopmdpy/pio.py \
             geopmdpy/runtime.py \
             geopmdpy/service.py \
             geopmdpy/session.py \
             geopmdpy/topo.py \
             geopmdpy/schemas.py \
             geopmdpy/schemas.py.in \
             geopmdpy/system_files.py \
             geopmdpy/version.py \
             geopmdpy/version.py.in \
             json_schemas/active_sessions.schema.json \
             json_schemas/msrs.schema.json \
             json_schemas/saved_controls.schema.json \
             geopmsession \
             requirements.txt \
             setup.py \
             src/geopm/json11.hpp \
             geopm.service \
             AUTHORS \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.rst \
             # end

geopmread_SOURCES = src/geopmread_main.cpp
geopmread_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11
geopmread_LDADD = libgeopmd.la
geopmwrite_SOURCES = src/geopmwrite_main.cpp
geopmwrite_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11
geopmwrite_LDADD = libgeopmd.la

# Add ABI version
libgeopmd_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

# MSR json definitions. From each json file, generate a same-prefixed cpp file,
# and use {arch}_msr_json as the variable identifier
# E.g., src/msr_data_skx.cpp is made from src/msr_data_skx.json and names the
# json variable skx_msr_json
msr_cpp_files = \
                src/msr_data_arch.cpp \
                src/msr_data_hsx.cpp \
                src/msr_data_knl.cpp \
                src/msr_data_skx.cpp \
                src/msr_data_snb.cpp \
                # end
EXTRA_DIST += \
	      src/msr_data_arch.json \
	      src/msr_data_hsx.json \
	      src/msr_data_knl.json \
	      src/msr_data_skx.json \
	      src/msr_data_snb.json \
	      src/msr_data.cpp.in \
	      # end

libgeopmd_la_SOURCES = $(include_HEADERS) \
                       $(geopminclude_HEADERS) \
                       contrib/json11/json11.cpp \
                       contrib/json11/json11.hpp \
                       src/GPUTopo.cpp \
                       src/GPUTopo.hpp \
                       src/GPUTopoNull.cpp \
                       src/GPUTopoNull.hpp \
                       src/Agg.cpp \
                       src/BatchClient.cpp \
                       src/BatchClient.hpp \
                       src/BatchServer.cpp \
                       src/BatchServer.hpp \
                       src/BatchStatus.cpp \
                       src/BatchStatus.hpp \
                       src/CNLIOGroup.cpp \
                       src/CNLIOGroup.hpp \
                       src/CombinedSignal.cpp \
                       src/CombinedSignal.hpp \
                       src/Control.hpp \
                       src/CpuinfoIOGroup.cpp \
                       src/CpuinfoIOGroup.hpp \
                       src/DerivativeSignal.cpp \
                       src/DerivativeSignal.hpp \
                       src/DifferenceSignal.cpp \
                       src/DifferenceSignal.hpp \
                       src/DCGMDevicePool.hpp \
                       src/DCGMIOGroup.hpp \
                       src/DCGMIOGroup.cpp \
                       src/DomainControl.cpp \
                       src/DomainControl.hpp \
                       src/Exception.cpp \
                       src/Helper.cpp \
                       src/IOGroup.cpp \
                       src/LevelZeroGPUTopo.cpp \
                       src/LevelZeroGPUTopo.hpp \
                       src/LevelZeroDevicePool.cpp \
                       src/LevelZeroDevicePool.hpp \
                       src/LevelZeroDevicePoolImp.hpp \
                       src/LevelZeroIOGroup.cpp \
                       src/LevelZeroIOGroup.hpp \
                       src/LevelZeroImp.hpp \
                       src/LevelZeroSignal.cpp \
                       src/LevelZeroSignal.hpp \
                       src/MSR.cpp \
                       src/MSR.hpp \
                       src/MSRFieldControl.cpp \
                       src/MSRFieldControl.hpp \
                       src/MSRFieldSignal.cpp \
                       src/MSRFieldSignal.hpp \
                       src/MSRIO.cpp \
                       src/MSRIO.hpp \
                       src/MSRIOImp.hpp \
                       src/MSRIOGroup.cpp \
                       src/MSRPath.cpp \
                       src/MSRPath.hpp \
                       src/MultiplicationSignal.cpp \
                       src/MultiplicationSignal.hpp \
                       src/NVMLGPUTopo.cpp \
                       src/NVMLGPUTopo.hpp \
                       src/NVMLDevicePool.hpp \
                       src/NVMLIOGroup.cpp \
                       src/NVMLIOGroup.hpp \
                       src/PlatformIO.cpp \
                       src/PlatformIOImp.hpp \
                       src/PlatformTopo.cpp \
                       src/PlatformTopoImp.hpp \
                       src/POSIXSignal.cpp \
                       src/POSIXSignal.hpp \
                       src/RawMSRSignal.cpp \
                       src/RawMSRSignal.hpp \
                       src/SaveControl.cpp \
                       src/SaveControl.hpp \
                       src/SDBus.hpp \
                       src/SDBusMessage.hpp \
                       src/ServiceIOGroup.cpp \
                       src/ServiceIOGroup.hpp \
                       src/ServiceProxy.cpp \
                       src/ServiceProxy.hpp \
                       src/SharedMemory.cpp \
                       src/SharedMemoryImp.hpp \
                       src/SharedMemoryScopedLock.cpp \
                       src/Signal.hpp \
                       src/SSTIO.cpp \
                       src/SSTIoctl.cpp \
                       src/SSTIoctlImp.hpp \
                       src/SSTIoctl.hpp \
                       src/SSTIOImp.hpp \
                       src/SSTIO.hpp \
                       src/SSTControl.cpp \
                       src/SSTControl.hpp \
                       src/SSTIOGroup.cpp \
                       src/SSTIOGroup.hpp \
                       src/SSTSignal.cpp \
                       src/SSTSignal.hpp \
                       src/TimeIOGroup.cpp \
                       src/TimeIOGroup.hpp \
                       src/TimeSignal.cpp \
                       src/TimeSignal.hpp \
                       src/TimeZero.cpp \
                       src/geopm_hash.c \
                       src/geopm_plugin.cpp \
                       src/geopm_sched.c \
                       src/geopm_version.c \
                       $(msr_cpp_files) \
                       # end

systemd_source_files = src/SDBus.cpp \
                       src/SDBusMessage.cpp \
                       # end
if ENABLE_SYSTEMD
    libgeopmd_la_SOURCES += $(systemd_source_files)
    EXTRA_DIST += src/SDBusNull.cpp
else
    libgeopmd_la_SOURCES += src/SDBusNull.cpp
    EXTRA_DIST += $(systemd_source_files)
endif



nvml_source_files = src/NVMLDevicePool.cpp \
                    src/NVMLDevicePoolImp.hpp \
                    #end

levelzero_source_files = src/LevelZero.cpp \
                         src/LevelZero.hpp \
                         #end

dcgm_source_files = src/DCGMDevicePool.cpp \
                    src/DCGMDevicePoolImp.hpp \
                    #end

if ENABLE_LEVELZERO
    libgeopmd_la_SOURCES += $(levelzero_source_files)
    EXTRA_DIST += src/LevelZeroThrow.cpp
else
    libgeopmd_la_SOURCES += src/LevelZeroThrow.cpp
    EXTRA_DIST += $(levelzero_source_files)
endif

if ENABLE_NVML
    libgeopmd_la_SOURCES += $(nvml_source_files)
    EXTRA_DIST += src/NVMLDevicePoolThrow.cpp
else
    libgeopmd_la_SOURCES += src/NVMLDevicePoolThrow.cpp
    EXTRA_DIST += $(nvml_source_files)
endif

if ENABLE_DCGM
    libgeopmd_la_SOURCES += $(dcgm_source_files)
    EXTRA_DIST += src/DCGMDevicePoolThrow.cpp
else
    libgeopmd_la_SOURCES += src/DCGMDevicePoolThrow.cpp
    EXTRA_DIST += $(dcgm_source_files)
endif

io.github.geopm.xml:
# Update io.github.geopm.xml in case API documentation changes
# Remove the file before executing target
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	$(PYTHON) -m geopmdpy.dbus_xml > $@

$(msr_cpp_files): %.cpp: %.json src/msr_data.cpp.in
	sed -e '/@MSR_JSON_CONTENTS@/ {' \
	    -e "r $<" \
	    -e 'd' -e '}' \
	    -e "s/@MSR_JSON_IDENTIFIER@/$(lastword $(subst _, ,$*))_msr_json/g" \
	src/msr_data.cpp.in > $@

$(abs_srcdir)/geopmdpy/version.py:
# Move version.py into source for out of place builds
	cp geopmdpy/version.py $@

install-python: setup.py $(abs_srcdir)/geopmdpy/version.py
	cd $(abs_srcdir) && $(PYTHON) ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)

clean-local-python: setup.py
	$(PYTHON) ./setup.py clean --all

# RPM TARGET
rpm_topdir ?= $(HOME)/rpmbuild
rpm: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-service-$(VERSION).tar.gz $(rpm_topdir)/SOURCES/geopm-service.tar.gz
	cp geopm-service.spec $(rpm_topdir)/SPECS/geopm-service.spec
	rpmbuild $(rpmbuild_flags) -ba $(rpm_topdir)/SPECS/geopm-service.spec

checkprogs: gtest-checkprogs geopmdpy-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)

all-local: $(all_local)

all_local =

CLEAN_LOCAL_TARGETS = clean-local-python

clean-local: $(CLEAN_LOCAL_TARGETS)

PHONY_TARGETS = clean-local-python doxygen

.PHONY: $(PHONY_TARGETS)

# INCLUDES
check_PROGRAMS =
check_LTLIBRARIES =
TESTS =
BUILT_SOURCES =
DISTCLEANFILES =
CLEANFILES = $(msr_cpp_files)

include test/Makefile.mk
include geopmdpy_test/Makefile.mk
include docs/Makefile.mk
include integration/Makefile.mk
