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
               -DGEOPM_DEFAULT_PLUGIN_PATH=\"$(libdir)/geopm\" \
               -D_POSIX_C_SOURCE=200809L

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmd.la

bin_PROGRAMS = geopmread \
               geopmwrite \
               # end

include_HEADERS = src/geopm.h \
                  src/geopm_debug.hpp \
                  src/geopm_error.h \
                  src/geopm_hash.h \
                  src/geopm_internal.h \
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
dist: dist-googletest

install-plugin-dir:
	$(INSTALL) -d $(DESTDIR)/$(pkglibdir)

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
             integration/README.md \
             integration/build_dasbus.sh \
             integration/install_service.sh \
             integration/test/__main__.py \
             integration/test/TestBashExamples.py \
             integration/test/check_write_session.sh \
             integration/test/test_controller.py \
             integration/test/test_controller.sh \
             integration/test/test_sst_priority.sh \
             integration/test/test_su_give_access.sh \
             geopmaccess \
             geopmd \
             geopmdpy/__init__.py \
             geopmdpy/__main__.py \
             geopmdpy/access.py \
             geopmdpy/dbus_xml.py \
             geopmdpy/error.py \
             geopmdpy/pio.py \
             geopmdpy/runtime.py \
             geopmdpy/service.py \
             geopmdpy/session.py \
             geopmdpy/topo.py \
             geopmdpy/version.py \
             geopmdpy/version.py.in \
             geopmsession \
             requirements.txt \
             setup.py \
             src/geopm/json11.hpp \
             geopm.service \
             AUTHORS \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.rst \
             COPYING \
             README.rst \
             VERSION \
             # end

geopmread_SOURCES = src/geopmread_main.cpp
geopmread_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11
geopmread_LDADD = libgeopmd.la
geopmwrite_SOURCES = src/geopmwrite_main.cpp
geopmwrite_CXXFLAGS = $(AM_CXXFLAGS) -std=c++11
geopmwrite_LDADD = libgeopmd.la

# Add ABI version
libgeopmd_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

libgeopmd_la_SOURCES = $(include_HEADERS) \
                       $(geopminclude_HEADERS) \
                       contrib/json11/json11.cpp \
                       contrib/json11/json11.hpp \
                       src/AcceleratorTopo.cpp \
                       src/AcceleratorTopo.hpp \
                       src/AcceleratorTopoNull.cpp \
                       src/AcceleratorTopoNull.hpp \
                       src/Agg.cpp \
                       src/BatchStatus.cpp \
                       src/BatchStatus.hpp \
                       src/BatchServer.cpp \
                       src/BatchServer.hpp \
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
                       src/DomainControl.cpp \
                       src/DomainControl.hpp \
                       src/Exception.cpp \
                       src/Helper.cpp \
                       src/IOGroup.cpp \
                       src/LevelZeroAcceleratorTopo.cpp \
                       src/LevelZeroAcceleratorTopo.hpp \
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
                       src/NVMLAcceleratorTopo.cpp \
                       src/NVMLAcceleratorTopo.hpp \
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
                       src/msr_data_arch.cpp \
                       src/msr_data_hsx.cpp \
                       src/msr_data_knl.cpp \
                       src/msr_data_skx.cpp \
                       src/msr_data_snb.cpp \
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

io.github.geopm.xml:
# Update io.github.geopm.xml in case API documentation changes
# Remove the file before executing target
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	$(PYTHON) -m geopmdpy.dbus_xml > $@


$(abs_srcdir)/geopmdpy/version.py:
# Move version.py into source for out of place builds
	cp geopmdpy/version.py $@

install-python: setup.py $(abs_srcdir)/geopmdpy/version.py
	cd $(abs_srcdir) && $(PYTHON) ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)

clean-local-python: setup.py
	$(PYTHON) ./setup.py clean --all

# DOXYGEN TARGETS
dox-public/Doxyfile: $(top_srcdir)/dox/Doxyfile.in
	mkdir -p dox-public
	sed -e "s|@DOX_VERSION@|$(VERSION)|g" \
	    -e "s|@DOX_OUTPUT@|$(abs_builddir)/dox-public|g" \
	    -e "s|@DOX_INPUT@|dox/blurb.md $(include_HEADERS) $(geopminclude_HEADERS)|g" \
        $^ > $@

dox/Doxyfile: $(top_srcdir)/dox/Doxyfile.in
	mkdir -p dox
	sed -e "s|@DOX_VERSION@|$(VERSION)|g" \
	    -e "s|@DOX_OUTPUT@|$(abs_builddir)/dox|g" \
	    -e "s|@DOX_INPUT@|dox/blurb.md src|g" \
        $^ > $@

doxygen: dox-public/Doxyfile dox/Doxyfile
	cd $(top_srcdir) && doxygen $(abs_builddir)/dox/Doxyfile
	cd $(top_srcdir) && doxygen $(abs_builddir)/dox-public/Doxyfile


# RPM TARGET
rpm_topdir ?= $(HOME)/rpmbuild
rpm: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-service-$(VERSION).tar.gz $(rpm_topdir)/SOURCES/geopm-service.tar.gz
	cp geopm-service.spec $(rpm_topdir)/SPECS/geopm-service.spec
	rpmbuild $(rpmbuild_flags) -ba $(rpm_topdir)/SPECS/geopm-service.spec

checkprogs: gtest-checkprogs geopmdpy-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)

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

include test/Makefile.mk
include geopmdpy_test/Makefile.mk
include docs/Makefile.mk
