#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GLOBAL SETTINGS
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_srcdir)/include \
               -DGEOPM_DEFAULT_PLUGIN_PATH=\"$(libdir)/geopm\" \
               -D_POSIX_C_SOURCE=200809L

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmd.la

bin_PROGRAMS = geopmread \
               geopmwrite \
               # end

include_HEADERS = include/geopm_debug.hpp \
                  include/geopm_error.h \
                  include/geopm_field.h \
                  include/geopm_hash.h \
                  include/geopm_hint.h \
                  include/geopm_pio.h \
                  include/geopm_plugin.hpp \
                  include/geopm_sched.h \
                  include/geopm_shmem.h \
                  include/geopm_time.h \
                  include/geopm_topo.h \
                  include/geopm_version.h \
                  # end

geopmincludedir = $(includedir)/geopm

geopminclude_HEADERS = include/geopm/json11.hpp \
                       include/geopm/Agg.hpp \
                       include/geopm/Cpuid.hpp \
                       include/geopm/CircularBuffer.hpp \
                       include/geopm/Exception.hpp \
                       include/geopm/Helper.hpp \
                       include/geopm/IOGroup.hpp \
                       include/geopm/MSRIOGroup.hpp \
                       include/geopm/PlatformIO.hpp \
                       include/geopm/PlatformTopo.hpp \
                       include/geopm/PluginFactory.hpp \
                       include/geopm/ServiceProxy.hpp \
                       include/geopm/SharedMemory.hpp \
                       include/geopm/SharedMemoryScopedLock.hpp \
                       # end

install-exec-hook: install-plugin-dir
dist: dist-googletest

install-plugin-dir:
	$(INSTALL) -d $(DESTDIR)/$(libdir)/geopm

dist_doc_DATA = COPYING \
                README.rst \
                VERSION \
                # end

EXTRA_DIST = .gitignore \
             autogen.sh \
             Makefile.am \
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
             debian/rules \
             dox/blurb.md \
             dox/Doxyfile.in \
             geopm-service.spec.in \
             io.github.geopm.conf \
             io.github.geopm.xml \
             geopm.service \
             AUTHORS \
             ChangeLog \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.rst \
             json_data/msr_data_arch.json \
             json_data/msr_data_hsx.json \
             json_data/msr_data_knl.json \
             json_data/msr_data_skx.json \
             json_data/msr_data_snb.json \
             json_data/msr_data_spr.json \
             json_data/sysfs_attributes_cpufreq.json \
             src/json_data.cpp.in \
             # end

geopmread_SOURCES = src/geopmread_main.cpp
geopmread_CXXFLAGS = $(AM_CXXFLAGS) -std=c++17
geopmread_LDADD = libgeopmd.la
geopmwrite_SOURCES = src/geopmwrite_main.cpp
geopmwrite_CXXFLAGS = $(AM_CXXFLAGS) -std=c++17
geopmwrite_LDADD = libgeopmd.la

# Add ABI version
libgeopmd_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

# MSR JSON definitions. From each JSON file, generate a same-prefixed cpp file,
# and use {arch}_msr_json as the variable identifier
# E.g., src/msr_data_skx.cpp is made from src/msr_data_skx.json and names the
# JSON variable skx_msr_json
msr_cpp_files = \
                src/msr_data_arch.cpp \
                src/msr_data_hsx.cpp \
                src/msr_data_knl.cpp \
                src/msr_data_skx.cpp \
                src/msr_data_snb.cpp \
                src/msr_data_spr.cpp \
                # end

# Sysfs JSON definitions. From each JSON file, generate a same-prefixed cpp file,
# and use {driver}_sysfs_json as the variable identifier
# E.g., src/sysfs_attributes_cpufreq.cpp is made from
# src/sysfs_attributes_cpufreq.json and names the JSON variable cpufreq_sysfs_json.
sysfs_cpp_files = \
		  src/sysfs_attributes_cpufreq.cpp \
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
                       src/UniqueFd.cpp \
                       src/UniqueFd.hpp \
                       src/geopm_hash.cpp \
                       src/geopm_plugin.cpp \
                       src/geopm_sched.c \
                       src/geopm_shmem.cpp \
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

$(msr_cpp_files): src/%.cpp: json_data/%.json src/json_data.cpp.in
	sed -e '/@JSON_CONTENTS@/ {' \
	    -e "r $<" \
	    -e 'd' -e '}' \
	    -e "s/@JSON_IDENTIFIER@/$(lastword $(subst _, ,$*))_msr_json/g" \
	src/json_data.cpp.in > $@

$(sysfs_cpp_files): src/%.cpp: json_data/%.json src/json_data.cpp.in
	sed -e '/@JSON_CONTENTS@/ {' \
	    -e "r $<" \
	    -e 'd' -e '}' \
	    -e "s/@JSON_IDENTIFIER@/$(lastword $(subst _, ,$*))_sysfs_json/g" \
	src/json_data.cpp.in > $@


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
checkprogs: $(check_PROGRAMS) $(check_LTLIBRARIES)
else
checkprogs: $(check_PROGRAMS) $(check_LTLIBRARIES)
endif

all-local: $(all_local)

all_local =

CLEAN_LOCAL_TARGETS =
if ENABLE_FUZZTESTS
CLEAN_LOCAL_TARGETS += clean-local-fuzztest
endif

clean-local: $(CLEAN_LOCAL_TARGETS)

PHONY_TARGETS = doxygen rpm deb

# INCLUDES
check_PROGRAMS =
check_LTLIBRARIES =
TESTS =
BUILT_SOURCES =
DISTCLEANFILES =
CLEANFILES = $(msr_cpp_files) $(sysfs_cpp_files)

include test/Makefile.mk

include shell_completion/Makefile.mk

include fuzz_test/Makefile.mk

.PHONY: $(PHONY_TARGETS)
