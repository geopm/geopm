#  Copyright (c) 2015 - 2023, Intel Corporation
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
                  src/geopm_shmem.h \
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
                       src/geopm/ServiceProxy.hpp \
                       src/geopm/SharedMemory.hpp \
                       src/geopm/SharedMemoryScopedLock.hpp \
                       # end

install-exec-hook: install-python install-plugin-dir
dist: dist-googletest base_man

install-plugin-dir:
	$(INSTALL) -d $(DESTDIR)/$(libdir)/geopm

dist_doc_DATA = COPYING \
                COPYING-TPP \
                README.rst \
                VERSION \
                # end

dist_man_MANS = docs/build/man/geopm.7 \
                docs/build/man/GEOPM_CXX_MAN_Agg.3 \
                docs/build/man/GEOPM_CXX_MAN_CircularBuffer.3 \
                docs/build/man/GEOPM_CXX_MAN_CNLIOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_CpuinfoIOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_Exception.3 \
                docs/build/man/GEOPM_CXX_MAN_Helper.3 \
                docs/build/man/GEOPM_CXX_MAN_IOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_MSRIO.3 \
                docs/build/man/GEOPM_CXX_MAN_MSRIOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_PlatformIO.3 \
                docs/build/man/GEOPM_CXX_MAN_PlatformTopo.3 \
                docs/build/man/GEOPM_CXX_MAN_PluginFactory.3 \
                docs/build/man/GEOPM_CXX_MAN_SampleAggregator.3 \
                docs/build/man/GEOPM_CXX_MAN_SharedMemory.3 \
                docs/build/man/GEOPM_CXX_MAN_TimeIOGroup.3 \
                docs/build/man/geopm_error.3 \
                docs/build/man/geopm_field.3 \
                docs/build/man/geopm_hash.3 \
                docs/build/man/geopm_pio.3 \
                docs/build/man/geopm_pio.7 \
                docs/build/man/geopm_pio_cnl.7 \
                docs/build/man/geopm_pio_const_config.7 \
                docs/build/man/geopm_pio_cpuinfo.7 \
                docs/build/man/geopm_pio_dcgm.7 \
                docs/build/man/geopm_pio_levelzero.7 \
                docs/build/man/geopm_pio_nvml.7 \
                docs/build/man/geopm_pio_profile.7 \
                docs/build/man/geopm_pio_service.7 \
                docs/build/man/geopm_pio_sst.7 \
                docs/build/man/geopm_pio_time.7 \
                docs/build/man/geopm_pio_msr.7 \
                docs/build/man/geopmread.1 \
                docs/build/man/geopm_report.7 \
                docs/build/man/geopm_sched.3 \
                docs/build/man/geopm_time.3 \
                docs/build/man/geopm_topo.3 \
                docs/build/man/geopm_version.3 \
                docs/build/man/geopmwrite.1 \
                docs/build/man/geopmaccess.1 \
                docs/build/man/geopmsession.1 \
                docs/build/man/geopmdpy.7 \
                # end

EXTRA_DIST = .gitignore \
             Makefile.am \
             autogen.sh \
             configure.ac \
             contrib/json11/LICENSE.txt \
             contrib/json11/README.md \
             debian/changelog \
             debian/changelog.in \
             debian/control \
             debian/copyright \
             debian/geopm.service \
             debian/geopm-service.dirs \
             debian/geopm-service.install \
             debian/geopm-service.postinst \
             debian/libgeopmd-dev.dirs \
             debian/libgeopmd-dev.install \
             debian/libgeopmd2.install \
             debian/python3-geopmdpy.install \
             debian/rules \
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
             geopmdpy/loop.py \
             geopmdpy/pio.py \
             geopmdpy/restorable_file_writer.py \
             geopmdpy/service.py \
             geopmdpy/session.py \
             geopmdpy/shmem.py \
             geopmdpy/topo.py \
             geopmdpy/schemas.py \
             geopmdpy/schemas.py.in \
             geopmdpy/system_files.py \
             geopmdpy/version.py \
             geopmdpy/version.py.in \
             geopmsession \
             docs/json_schemas/active_sessions.schema.json \
             docs/json_schemas/const_config_io.schema.json \
             docs/json_schemas/msrs.schema.json \
             docs/json_schemas/saved_controls.schema.json \
             docs/json_schemas/domainnetmap_neural_net.schema.json \
             docs/json_schemas/geopmagent_policy.schema.json \
             docs/json_schemas/geopmbench_config.schema.json \
             docs/json_schemas/regionhintrecommender_fmap.schema.json \
             requirements.txt \
             setup.py \
             src/geopm/json11.hpp \
             geopm.service \
             AUTHORS \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.rst \
             # end

geopmread_SOURCES = src/geopmread_main.cpp
geopmread_CXXFLAGS = $(AM_CXXFLAGS) -std=c++17
geopmread_LDADD = libgeopmd.la
geopmwrite_SOURCES = src/geopmwrite_main.cpp
geopmwrite_CXXFLAGS = $(AM_CXXFLAGS) -std=c++17
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
                src/msr_data_spr.cpp \
                # end
EXTRA_DIST += \
	      docs/json_data/msr_data_arch.json \
	      docs/json_data/msr_data_hsx.json \
	      docs/json_data/msr_data_knl.json \
	      docs/json_data/msr_data_skx.json \
	      docs/json_data/msr_data_snb.json \
	      docs/json_data/msr_data_spr.json \
	      src/json_data.cpp.in \
	      # end

# Sysfs json definitions. From each json file, generate a same-prefixed cpp file,
# and use {driver}_sysfs_json as the variable identifier
# E.g., src/sysfs_attributes_cpufreq.cpp is made from
# src/sysfs_attributes_cpufreq.json and names the json variable cpufreq_sysfs_json.
sysfs_cpp_files = \
		  src/sysfs_attributes_cpufreq.cpp \
		  #
EXTRA_DIST += \
	      src/sysfs_attributes_cpufreq.json \
	      #

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
                       src/CombinedControl.cpp \
                       src/CombinedControl.hpp \
                       src/CombinedSignal.cpp \
                       src/CombinedSignal.hpp \
                       src/ConstConfigIOGroup.cpp \
                       src/ConstConfigIOGroup.hpp \
                       src/Control.hpp \
                       src/Cpuid.cpp \
                       src/Cpuid.hpp \
                       src/CpuinfoIOGroup.cpp \
                       src/CpuinfoIOGroup.hpp \
                       src/SysfsIOGroup.cpp \
                       src/SysfsIOGroup.hpp \
                       src/CpufreqSysfsDriver.cpp \
                       src/CpufreqSysfsDriver.hpp \
                       src/DCGMDevicePool.hpp \
                       src/DCGMIOGroup.hpp \
                       src/DCGMIOGroup.cpp \
                       src/DerivativeSignal.cpp \
                       src/DerivativeSignal.hpp \
                       src/DifferenceSignal.cpp \
                       src/DifferenceSignal.hpp \
                       src/RatioSignal.cpp \
                       src/RatioSignal.hpp \
                       src/DomainControl.cpp \
                       src/DomainControl.hpp \
                       src/Exception.cpp \
                       src/GEOPMHint.cpp \
                       src/Helper.cpp \
                       src/IOGroup.cpp \
                       src/IOUring.cpp \
                       src/IOUring.hpp \
                       src/IOUringFallback.cpp \
                       src/IOUringFallback.hpp \
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
                       src/SecurePath.cpp \
                       src/SecurePath.hpp \
                       src/ServiceIOGroup.cpp \
                       src/ServiceIOGroup.hpp \
                       src/ServiceProxy.cpp \
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
                       src/SysfsDriver.cpp \
                       src/SysfsDriver.hpp \
                       src/TimeIOGroup.cpp \
                       src/TimeIOGroup.hpp \
                       src/TimeSignal.cpp \
                       src/TimeSignal.hpp \
                       src/TimeZero.cpp \
                       src/geopm_hash.cpp \
                       src/geopm_plugin.cpp \
                       src/geopm_sched.c \
                       src/geopm_shmem.cpp \
                       src/geopm_shmem.h \
                       src/geopm_version.c \
                       $(msr_cpp_files) \
                       $(sysfs_cpp_files) \
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

io_uring_source_files = src/IOUringImp.cpp \
                        src/IOUringImp.hpp \
                        #end
if ENABLE_IO_URING
    libgeopmd_la_SOURCES += $(io_uring_source_files)
else
    EXTRA_DIST += $(io_uring_source_files)
endif

io.github.geopm.xml:
# Update io.github.geopm.xml in case API documentation changes
# Remove the file before executing target
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	$(PYTHON) -m geopmdpy.dbus_xml > $@

$(msr_cpp_files): src/%.cpp: docs/json_data/%.json src/json_data.cpp.in
	sed -e '/@JSON_CONTENTS@/ {' \
	    -e "r $<" \
	    -e 'd' -e '}' \
	    -e "s/@JSON_IDENTIFIER@/$(lastword $(subst _, ,$*))_msr_json/g" \
	src/json_data.cpp.in > $@

$(sysfs_cpp_files): %.cpp: %.json src/json_data.cpp.in
	sed -e '/@JSON_CONTENTS@/ {' \
	    -e "r $<" \
	    -e 'd' -e '}' \
	    -e "s/@JSON_IDENTIFIER@/$(lastword $(subst _, ,$*))_sysfs_json/g" \
	src/json_data.cpp.in > $@

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
	cp geopm-service-$(VERSION).tar.gz $(rpm_topdir)/SOURCES
	cp geopm-service.spec $(rpm_topdir)/SPECS
	rpmbuild $(rpmbuild_flags) -ba $(rpm_topdir)/SPECS/geopm-service.spec

deb: dist
	rm -rf geopm-service-$(VERSION)
	tar xvf geopm-service-$(VERSION).tar.gz
	cd geopm-service-$(VERSION) && dpkg-buildpackage -us -uc
	rm -rf geopm-service-$(VERSION)

if !ENABLE_SANITIZERS
checkprogs: gtest-checkprogs geopmdpy-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)
else
checkprogs: gtest-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)
endif

all-local: $(all_local)

all_local =

CLEAN_LOCAL_TARGETS = clean-local-python
if ENABLE_FUZZTESTS
CLEAN_LOCAL_TARGETS += clean-local-fuzztest
endif

clean-local: $(CLEAN_LOCAL_TARGETS)

PHONY_TARGETS = clean-local-python doxygen rpm deb

# INCLUDES
check_PROGRAMS =
check_LTLIBRARIES =
TESTS =
BUILT_SOURCES =
DISTCLEANFILES =
CLEANFILES = $(msr_cpp_files) $(sysfs_cpp_files)

include test/Makefile.mk

if !ENABLE_SANITIZERS
include geopmdpy_test/Makefile.mk
include integration/Makefile.mk
endif

include shell_completion/Makefile.mk

include fuzz_test/Makefile.mk

include docs/Makefile.mk

.PHONY: $(PHONY_TARGETS)
