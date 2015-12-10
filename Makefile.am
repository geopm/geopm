#  Copyright (c) 2015, Intel Corporation
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
AM_CPPFLAGS = -I$(top_srcdir)/src -DPLUGINDIR=\"$(pkglibdir)\" -D_POSIX_C_SOURCE=200112L

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmpolicy.la
if ENABLE_MPI
    lib_LTLIBRARIES += libgeopm.la
    pkglib_LTLIBRARIES = libgoverning_decider.la
endif

bin_PROGRAMS = geopmpolicy
if ENABLE_MPI
    bin_PROGRAMS += geopmctl
endif


include_HEADERS = src/geopm.h \
                  src/geopm_error.h \
                  src/geopm_mpi_pcontrol.h \
                  src/geopm_policy.h \
                  src/geopm_message.h \
                  src/geopm_version.h \
                  src/geopm_plugin.h \
                  # end

dist_man_MANS = man/geopm.7 \
                man/geopmctl.1 \
                man/geopm_ctl_c.3 \
                man/geopm_error.3 \
                man/geopmkey.1 \
                man/geopm_omp.3 \
                man/geopmpolicy.1 \
                man/geopm_policy_c.3 \
                man/geopm_prof_c.3 \
                man/geopm_version.3 \
                # end

dist_doc_DATA = README \
                COPYING \
                VERSION \
                # end

EXTRA_DIST = .gitignore \
             autogen.sh \
             ChangeLog \
             configure.ac \
             CONTRIBUTING \
             copying_headers/header.BSD3-intel \
             copying_headers/header.BSD3-llnl \
             copying_headers/MANIFEST.BSD3-llnl \
             copying_headers/MANIFEST.EXEMPT \
             copying_headers/MANIFEST.BSD3-intel \
             copying_headers/test-license \
             dox/Doxyfile \
             examples/README \
             examples/threaded_step_example.c \
             geopm.spec.mk \
             geopm-$(VERSION).spec \
             Makefile.am \
             MANIFEST \
             README.md \
             test/MPIControllerTest.cpp \
             test/MPITreeCommunicatorTest.cpp \
             test/no_omp_cpu.c \
             TODO \
             # end

# EXTEND FLAGS FOR MPI TARGETS
if ENABLE_MPI
    libgeopm_la_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    libgeopm_la_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS) -version-info $(geopm_abi_version)
    libgeopm_la_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    libgeopm_la_CXXFLAGS= $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)

    geopmctl_CPPFLAGS = $(CPPFLAGS) $(AM_CPPFLAGS) $(MPI_CFLAGS)
    geopmctl_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    geopmctl_CFLAGS = $(CFLAGS) $(AM_CFLAGS) $(MPI_CFLAGS)
    geopmctl_CXXFLAGS = $(CXXFLAGS) $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
endif

# EXTEND FLAGS FOR NON-MPI TARGETS
libgeopmpolicy_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

# ADD LIBRARY DEPENDENCIES FOR EXECUTABLES
geopmpolicy_LDADD = libgeopmpolicy.la
if ENABLE_MPI
    geopmctl_LDADD = libgeopm.la
endif

# SOURCE LISTS FOR EACH TARGET
libgeopmpolicy_la_SOURCES = src/CircularBuffer.cpp \
                            src/CircularBuffer.hpp \
                            src/Decider.cpp \
                            src/Decider.hpp \
                            src/DeciderFactory.cpp \
                            src/DeciderFactory.hpp \
                            src/Exception.cpp \
                            src/Exception.hpp \
                            src/GlobalPolicy.cpp \
                            src/GlobalPolicy.hpp \
                            src/HSXPlatformImp.cpp \
                            src/HSXPlatformImp.hpp \
                            src/IVTPlatformImp.cpp \
                            src/IVTPlatformImp.hpp \
                            src/Observation.cpp \
                            src/Observation.hpp \
                            src/PlatformImp.cpp \
                            src/PlatformImp.hpp \
                            src/Platform.cpp \
                            src/Platform.hpp \
                            src/PlatformFactory.cpp \
                            src/PlatformFactory.hpp \
                            src/PlatformTopology.cpp \
                            src/PlatformTopology.hpp \
                            src/Policy.cpp \
                            src/Policy.hpp \
                            src/PowerModel.cpp \
                            src/PowerModel.hpp \
                            src/RAPLPlatform.cpp \
                            src/RAPLPlatform.hpp \
                            src/Region.cpp \
                            src/Region.hpp \
                            src/SharedMemory.cpp \
                            src/SharedMemory.hpp \
                            src/geopm_error.h \
                            src/geopm_hash.c \
                            src/geopm_hash.h \
                            src/geopm_omp.c \
                            src/geopm_omp.h \
                            src/geopm_plugin.c \
                            src/geopm_plugin.h \
                            src/geopm_policy.h \
                            src/geopm_policy_message.c \
                            src/geopm_message.h \
                            src/geopm_version.c \
                            src/geopm_version.h \
                            # end

geopmpolicy_SOURCES = src/geopmpolicy_main.c \
                      src/geopm_policy.h \
                      src/geopm_message.h \
                      src/geopm_version.h \
                      # end


if ENABLE_MPI
    libgoverning_decider_la_SOURCES= src/Decider.hpp \
                                     src/GoverningDecider.cpp \
                                     src/GoverningDecider.hpp \
                                     src/geopm_plugin.h \
                                     # end

    libgeopm_la_SOURCES = src/CircularBuffer.cpp \
                          src/CircularBuffer.hpp \
                          src/Controller.cpp \
                          src/Controller.hpp \
                          src/Decider.cpp \
                          src/Decider.hpp \
                          src/DeciderFactory.cpp \
                          src/DeciderFactory.hpp \
                          src/Exception.cpp \
                          src/Exception.hpp \
                          src/GlobalPolicy.cpp \
                          src/GlobalPolicy.hpp \
                          src/HSXPlatformImp.cpp \
                          src/HSXPlatformImp.hpp \
                          src/IVTPlatformImp.cpp \
                          src/IVTPlatformImp.hpp \
                          src/Observation.cpp \
                          src/Observation.hpp \
                          src/LockingHashTable.hpp \
                          src/Platform.cpp \
                          src/PlatformFactory.cpp \
                          src/PlatformFactory.hpp \
                          src/Platform.hpp \
                          src/PlatformImp.cpp \
                          src/PlatformImp.hpp \
                          src/PlatformTopology.cpp \
                          src/PlatformTopology.hpp \
                          src/Policy.cpp \
                          src/Policy.hpp \
                          src/PowerModel.cpp \
                          src/PowerModel.hpp \
                          src/Profile.cpp \
                          src/Profile.hpp \
                          src/RAPLPlatform.cpp \
                          src/RAPLPlatform.hpp \
                          src/Region.cpp \
                          src/Region.hpp \
                          src/SharedMemory.cpp \
                          src/SharedMemory.hpp \
                          src/TreeCommunicator.cpp \
                          src/TreeCommunicator.hpp \
                          src/geopm_ctl_spawn.c \
                          src/geopm_error.h \
                          src/geopm_hash.c \
                          src/geopm_hash.h \
                          src/geopm_omp.c \
                          src/geopm_omp.h \
                          src/geopm_mpi_pcontrol.h \
                          src/geopm_plugin.c \
                          src/geopm_plugin.h \
                          src/geopm_policy_message.c \
                          src/geopm_message.h \
                          src/geopm_version.c \
                          src/geopm_version.h \
                          # end

    geopmctl_SOURCES = src/geopmctl_main.c \
                       src/geopm_version.h \
                       # end
endif

# CLEAN TARGETS
CLEANFILES = geopm-$(VERSION).spec
DISTCLEANFILES = VERSION MANIFEST

clean-local: clean-local-coverage clean-local-man

clean-local-coverage:
	rm -f `find -name \*.gcno` `find -name \*.gcda`

clean-local-man:
	rm -rf ruby man ronn/*.html ronn/*.1 ronn/*.3 ronn/*.7

# RPM TARGET
geopm-$(VERSION).spec:
	$(MAKE) version="$(VERSION)" -f geopm.spec.mk $@

rpm: dist
	$(MAKE) version="$(VERSION)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)" \
		-f geopm.spec.mk $@

# DOXYGEN TARGET
doxygen:
	doxygen dox/Doxyfile

# RONN TARGET
ruby/bin/ronn:
	mkdir -p ruby/bin
	which ronn >& /dev/null && \
	ln -s `which ronn` ruby/bin/ronn || \
	gem install ronn -i ruby

# MAN TARGET
$(dist_man_MANS): man/%: ruby/bin/ronn
	mkdir -p man;
	pushd  ronn; \
	../ruby/bin/ronn --date=`date "+%Y-%m-%d"` --organization="Intel Corporation" `basename $@`.ronn; \
	popd;
	cat ronn/header.txt ronn/`basename $@` | \
	    sed -e 's/\(#include.*\) \\fIhttp.*/\1\\fR/g' \
	    > $@;

# GH_PAGES TARGET
gh-pages: doxygen $(dist_man_MANS)
	mkdir -p gh-pages/dox
	mkdir -p gh-pages/man
	cp -rf dox/html/* gh-pages/dox/.
	cp ronn/*.html gh-pages/man/.

# INCLUDES
check_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BUILT_SOURCES =

include test/Makefile.mk
include examples/Makefile.mk

.PHONY: clean-local clean-local-coverage clean-local-man rpm doxygen gh_pages
