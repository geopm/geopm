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

# GLOBAL SETTINGS
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS += -I$(top_srcdir)/src \
               -DGEOPM_DEFAULT_PLUGIN_PATH=\"$(pkglibdir)\" \
               -D_POSIX_C_SOURCE=200809L

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmd.la

install-exec-hook: install-python install-plugin-dir

install-plugin-dir:
	$(INSTALL) -d $(DESTDIR)/$(pkglibdir)

EXTRA_DIST = .gitignore \
             Makefile.am \
             autogen.sh \
             configure.ac \
             contrib/json11/LICENSE.txt \
             contrib/json11/README.md \
             geopm-service.spec.in \
             io.github.geopm.conf \
             io.github.geopm.xml \
             geopmaccess \
             geopmd \
             geopmdpy/__init__.py \
             geopmdpy/__main__.py \
             geopmdpy/access.py \
             geopmdpy/error.py \
             geopmdpy/pio.py \
             geopmdpy/service.py \
             geopmdpy/session.py \
             geopmdpy/topo.py \
             geopmdpy/version.py \
             geopmdpy/version.py.in \
             geopmdpy_test/__main__.py \
             geopmdpy_test/test_service.py \
             geopmsession \
             requirements.txt \
             setup.py \
             test/README.md \
             test/install_service.sh \
             test/test_sst_priority.sh \
             test/test_su_give_access.sh \
             test/test_write_session.sh \
             geopm.service \
             AUTHORS \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.md \
             COPYING \
             README.md \
             TODO.md \
             VERSION \
             # end

# Add ABI version
libgeopmd_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

libgeopmd_la_SOURCES = contrib/json11/json11.cpp \
                       contrib/json11/json11.hpp \
                       src/AcceleratorTopo.cpp \
                       src/AcceleratorTopo.hpp \
                       src/AcceleratorTopoNull.cpp \
                       src/AcceleratorTopoNull.hpp \
                       src/Agg.cpp \
                       src/Agg.hpp \
                       src/CircularBuffer.hpp \
                       src/CNLIOGroup.cpp \
                       src/CNLIOGroup.hpp \
                       src/CombinedSignal.cpp \
                       src/CombinedSignal.hpp \
                       src/Control.hpp \
                       src/CpuinfoIOGroup.cpp \
                       src/CpuinfoIOGroup.hpp \
                       src/DBusServer.hpp \
                       src/DerivativeSignal.cpp \
                       src/DerivativeSignal.hpp \
                       src/DifferenceSignal.cpp \
                       src/DifferenceSignal.hpp \
                       src/DomainControl.cpp \
                       src/DomainControl.hpp \
                       src/Exception.cpp \
                       src/Exception.hpp \
                       src/Helper.cpp \
                       src/Helper.hpp \
                       src/IOGroup.cpp \
                       src/IOGroup.hpp \
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
                       src/MSRIOGroup.hpp \
                       src/MSRPath.cpp \
                       src/MSRPath.hpp \
                       src/NVMLAcceleratorTopo.cpp \
                       src/NVMLAcceleratorTopo.hpp \
                       src/NVMLDevicePool.hpp \
                       src/NVMLIOGroup.cpp \
                       src/NVMLIOGroup.hpp \
                       src/PlatformIO.cpp \
                       src/PlatformIO.hpp \
                       src/PlatformIOImp.hpp \
                       src/PlatformTopo.cpp \
                       src/PlatformTopo.hpp \
                       src/PlatformTopoImp.hpp \
                       src/PluginFactory.hpp \
                       src/RawMSRSignal.cpp \
                       src/RawMSRSignal.hpp \
                       src/SharedMemory.cpp \
                       src/SharedMemory.hpp \
                       src/SharedMemoryImp.hpp \
                       src/SharedMemoryScopedLock.cpp \
                       src/SharedMemoryScopedLock.hpp \
                       src/Signal.hpp \
                       src/SSTIO.cpp \
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
                       src/geopm.h \
                       src/geopm_debug.hpp \
                       src/geopm_error.h \
                       src/geopm_internal.h \
                       src/geopm_pio.h \
                       src/geopm_plugin.cpp \
                       src/geopm_plugin.hpp \
                       src/geopm_sched.c \
                       src/geopm_sched.h \
                       src/geopm_time.h \
                       src/geopm_topo.h \
                       src/geopm_version.c \
                       src/geopm_version.h \
                       src/msr_data_arch.cpp \
                       src/msr_data_hsx.cpp \
                       src/msr_data_knl.cpp \
                       src/msr_data_skx.cpp \
                       src/msr_data_snb.cpp \
                       # end

nvml_source_files = src/NVMLDevicePool.cpp \
                    src/NVMLDevicePoolImp.hpp \
                    # end

if ENABLE_NVML
    libgeopmd_la_SOURCES += $(nvml_source_files)
else
    libgeopmd_la_SOURCES += src/NVMLDevicePoolThrow.cpp
    EXTRA_DIST += $(nvml_source_files)
endif

install-python: setup.py
# Move version.py into source for out of place builds
	if [ ! -f $(abs_srcdir)/geopmdpy/version.py ]; then \
	    cp geopmdpy/version.py $(abs_srcdir)/geopmdpy/version.py; \
	fi
	cd $(abs_srcdir) && $(PYTHON) ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)


# RPM TARGET
rpm_topdir ?= $(HOME)/rpmbuild
rpm: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-service-$(VERSION).tar.gz $(rpm_topdir)/SOURCES/geopm-service.tar.gz
	cp geopm-service.spec $(rpm_topdir)/SPECS/geopm-service.spec
	rpmbuild $(rpmbuild_flags) -ba $(rpm_topdir)/SPECS/geopm-service.spec
