#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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
AM_CPPFLAGS += -I$(top_srcdir)/src -DGEOPM_PLUGIN_PATH=\"$(pkglibdir)\" -D_POSIX_C_SOURCE=200112L -DOMPI_SKIP_MPICXX

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmpolicy.la
bin_PROGRAMS = geopmpolicy
pkglib_LTLIBRARIES =
nodist_include_HEADERS =

if ENABLE_MPI
    lib_LTLIBRARIES += libgeopm.la
    bin_PROGRAMS += geopmctl
if ENABLE_FORTRAN
    lib_LTLIBRARIES += libgeopmfort.la
endif
endif

if !ENABLE_FORTRAN
install-fortran:
	-
endif


install-exec-hook: install-python install-fortran install-openmp
dist: dist-googletest dist-openmp


include_HEADERS = src/geopm.h \
                  src/geopm_error.h \
                  src/geopm_mpi_pcontrol.h \
                  src/geopm_policy.h \
                  src/geopm_message.h \
                  src/geopm_version.h \
                  src/geopm_plugin.h \
                  # end

if ENABLE_MPI
if ENABLE_FORTRAN

all-local: geopm.mod

install-fortran:
	$(INSTALL) -d $(DESTDIR)/$(libdir)/$(FC)/modules/geopm-`uname -m`
	$(INSTALL) geopm.mod $(DESTDIR)/$(libdir)/$(FC)/modules/geopm-`uname -m`

endif
endif

ronn_man = man/geopm.7 \
           man/geopmctl.1 \
           man/geopm_ctl_c.3 \
           man/geopm_error.3 \
           man/geopm_fortran.3 \
           man/geopmpy_launcher.1 \
           man/geopm_omp.3 \
           man/geopmplotter.1 \
           man/geopmpolicy.1 \
           man/geopm_policy_c.3 \
           man/geopm_prof_c.3 \
           man/geopm_version.3 \
           # end

dist_man_MANS = man/geopmaprun.1 \
                man/geopmsrun.1 \
                $(ronn_man) \
                # end


dist_doc_DATA = README \
                COPYING \
                VERSION \
                # end

EXTRA_DIST = .gitignore \
             .travis.yml \
             .travis_obs.sh \
             autogen.sh \
             ChangeLog \
             configure.ac \
             CONTRIBUTING.md \
             copying_headers/header.BSD3-intel \
             copying_headers/header.BSD3-llnl \
             copying_headers/header.BSD3-mvapich2 \
             copying_headers/header.NAS \
             copying_headers/MANIFEST.BSD3-llnl \
             copying_headers/MANIFEST.BSD3-intel \
             copying_headers/MANIFEST.BSD3-mvapich2 \
             copying_headers/MANIFEST.EXEMPT \
             copying_headers/MANIFEST.NAS \
             copying_headers/test-license \
             copying_headers/test-dist \
             dox/Doxyfile \
             geopm.spec.mk \
             geopm-$(VERSION).spec \
             Makefile.am \
             MANIFEST \
             README.md \
             ronn/geopm.7.ronn \
             ronn/geopmctl.1.ronn \
             ronn/geopm_ctl_c.3.ronn \
             ronn/geopm_error.3.ronn \
             ronn/geopm_fortran.3.ronn \
             ronn/geopmpy_launcher.1.ronn \
             ronn/geopm_omp.3.ronn \
             ronn/geopmplotter.1.ronn \
             ronn/geopmpolicy.1.ronn \
             ronn/geopm_policy_c.3.ronn \
             ronn/geopm_prof_c.3.ronn \
             ronn/geopm_version.3.ronn \
             ronn/header.txt \
             ronn/index.txt \
             src/Comm.hpp \
             test/geopm_static_modes_test.sh \
             TODO \
             tracker/track \
             $(TUTORIAL_DIST) \
             geopm-tutorial.tar.gz \
             # end

TUTORIAL_DIST = tutorial/Imbalancer.cpp \
                tutorial/imbalancer.h \
                tutorial/Makefile \
                tutorial/README.md \
                tutorial/ModelApplication.cpp \
                tutorial/ModelApplication.hpp \
                tutorial/ModelRegion.cpp \
                tutorial/ModelRegion.hpp \
                tutorial/tutorial_0.c \
                tutorial/tutorial_0.sh \
                tutorial/tutorial_1.c \
                tutorial/tutorial_1.sh \
                tutorial/tutorial_2.c \
                tutorial/tutorial_2.sh \
                tutorial/tutorial_3.c \
                tutorial/tutorial_3.sh \
                tutorial/tutorial_4.c \
                tutorial/tutorial_4.sh \
                tutorial/tutorial_5.c \
                tutorial/tutorial_5.sh \
                tutorial/tutorial_6.cpp \
                tutorial/tutorial_6.sh \
                tutorial/tutorial_6_config.json \
                tutorial/tutorial_balanced_policy.json \
                tutorial/tutorial_build_gnu.sh \
                tutorial/tutorial_build_intel.sh \
                tutorial/tutorial_env.sh \
                tutorial/tutorial_governed_policy.json \
                tutorial/tutorial_region.c \
                tutorial/tutorial_region.h \
                # end

# EXTEND FLAGS FOR MPI TARGETS
if ENABLE_MPI
    libgeopm_la_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS) -version-info $(geopm_abi_version)
    libgeopm_la_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    libgeopm_la_CXXFLAGS= $(AM_CXXFLAGS) $(MPI_CFLAGS)

    geopmctl_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS)
    geopmctl_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    geopmctl_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS)

if ENABLE_FORTRAN
    libgeopmfort_la_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
    libgeopmfort_la_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    libgeopmfort_la_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS) -version-info $(geopm_abi_version)
endif
endif

# EXTEND FLAGS FOR NON-MPI TARGETS
libgeopmpolicy_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

# ADD LIBRARY DEPENDENCIES FOR EXECUTABLES
geopmpolicy_LDADD = libgeopmpolicy.la
if ENABLE_MPI
    geopmctl_LDADD = libgeopm.la $(MPI_CLIBS)
    libgeopm_la_LIBADD = $(MPI_CLIBS)
if ENABLE_FORTRAN
    libgeopmfort_la_LIBADD = libgeopm.la $(MPI_FLIBS)
endif
endif

# SOURCE LISTS FOR EACH TARGET
libgeopmpolicy_la_SOURCES = src/CircularBuffer.hpp \
                            src/Decider.cpp \
                            src/Decider.hpp \
                            src/DeciderFactory.cpp \
                            src/DeciderFactory.hpp \
                            src/Environment.cpp \
                            src/Exception.cpp \
                            src/Exception.hpp \
                            src/GlobalPolicy.cpp \
                            src/GlobalPolicy.hpp \
                            src/KNLPlatformImp.cpp \
                            src/KNLPlatformImp.hpp \
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
                            src/PolicyFlags.cpp \
                            src/PolicyFlags.hpp \
                            src/ProfileTable.cpp \
                            src/ProfileTable.hpp \
                            src/RAPLPlatform.cpp \
                            src/RAPLPlatform.hpp \
                            src/Region.cpp \
                            src/Region.hpp \
                            src/SampleRegulator.cpp \
                            src/SampleRegulator.hpp \
                            src/SampleScheduler.cpp \
                            src/SampleScheduler.hpp \
                            src/SharedMemory.cpp \
                            src/SharedMemory.hpp \
                            src/SignalHandler.cpp \
                            src/StaticPolicyDecider.cpp \
                            src/StaticPolicyDecider.hpp \
                            src/Tracer.cpp \
                            src/Tracer.hpp \
                            src/XeonPlatformImp.cpp \
                            src/XeonPlatformImp.hpp \
                            src/geopm_env.h \
                            src/geopm_error.h \
                            src/geopm_hash.c \
                            src/geopm_hash.h \
                            src/geopm_message.c \
                            src/geopm_message.h \
                            src/geopm_omp.c \
                            src/geopm_omp.h \
                            src/geopm_plugin.c \
                            src/geopm_plugin.h \
                            src/geopm_policy.h \
                            src/geopm_time.h \
                            src/geopm_sched.h \
                            src/geopm_signal_handler.h \
                            src/geopm_version.c \
                            src/geopm_version.h \
                            # end

geopmpolicy_SOURCES = src/geopmpolicy_main.c \
                      src/geopm_policy.h \
                      src/geopm_message.h \
                      src/geopm_version.h \
                      # end


if ENABLE_MPI
    libgeopm_la_SOURCES = src/CircularBuffer.hpp \
                          src/Controller.cpp \
                          src/Controller.hpp \
                          src/Decider.cpp \
                          src/Decider.hpp \
                          src/DeciderFactory.cpp \
                          src/DeciderFactory.hpp \
                          src/Environment.cpp \
                          src/Exception.cpp \
                          src/Exception.hpp \
                          src/GlobalPolicy.cpp \
                          src/GlobalPolicy.hpp \
                          src/KNLPlatformImp.cpp \
                          src/KNLPlatformImp.hpp \
                          src/OMPT.cpp \
                          src/OMPT.hpp \
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
                          src/PolicyFlags.cpp \
                          src/PolicyFlags.hpp \
                          src/Profile.cpp \
                          src/Profile.hpp \
                          src/ProfileTable.cpp \
                          src/ProfileTable.hpp \
                          src/ProfileThread.cpp \
                          src/ProfileThread.hpp \
                          src/RAPLPlatform.cpp \
                          src/RAPLPlatform.hpp \
                          src/Region.cpp \
                          src/Region.hpp \
                          src/SampleRegulator.cpp \
                          src/SampleRegulator.hpp \
                          src/SampleScheduler.cpp \
                          src/SampleScheduler.hpp \
                          src/SharedMemory.cpp \
                          src/SharedMemory.hpp \
                          src/SignalHandler.cpp \
                          src/StaticPolicyDecider.cpp \
                          src/StaticPolicyDecider.hpp \
                          src/Tracer.cpp \
                          src/Tracer.hpp \
                          src/geopm_env.h \
                          src/TreeCommunicator.cpp \
                          src/TreeCommunicator.hpp \
                          src/XeonPlatformImp.cpp \
                          src/XeonPlatformImp.hpp \
                          src/geopm_ctl_spawn.c \
                          src/geopm_error.h \
                          src/geopm_hash.c \
                          src/geopm_hash.h \
                          src/geopm_omp.c \
                          src/geopm_omp.h \
                          src/geopm_message.c \
                          src/geopm_message.h \
                          src/geopm_mpi_pcontrol.h \
                          src/geopm_plugin.c \
                          src/geopm_plugin.h \
                          src/geopm_pmpi.h \
                          src/geopm_pmpi.c \
                          src/geopm_signal_handler.h \
                          src/geopm_sched.h \
                          src/geopm_time.h \
                          src/geopm_version.c \
                          src/geopm_version.h \
                          # end
if ENABLE_FORTRAN
    libgeopmfort_la_SOURCES = src/geopm.f90 \
                              src/geopm_pmpi_fortran.c \
                              #end
endif

    geopmctl_SOURCES = src/geopmctl_main.c \
                       src/geopm_version.h \
                       # end

endif

# CLEAN TARGETS
CLEANFILES = geopm-$(VERSION).spec
DISTCLEANFILES = VERSION MANIFEST

CLEAN_LOCAL_TARGETS= clean-local-coverage \
                     clean-local-man \
                     clean-local-fortran \
                     clean-local-doc \
                     clean-local-googletest \
                     clean-local-openmp \
                     #end

clean-local: $(CLEAN_LOCAL_TARGETS)

distclean-local: distclean-local-ruby

clean-local-coverage:
	rm -f `find -name \*.gcno` `find -name \*.gcda`

clean-local-man:
	rm -rf man ronn/*.html ronn/*.1 ronn/*.3 ronn/*.7

clean-local-fortran:
	rm -rf geopm.mod

distclean-local-ruby:
	rm -rf ruby

# FORTRAN MODULE TARGET
if ENABLE_FORTRAN
geopm.mod: src/geopm.f90
	$(FC) $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS) -c $<
endif

# RPM TARGET
geopm-$(VERSION).spec:
	$(MAKE) version="$(VERSION)" -f geopm.spec.mk $@

rpm: dist
	$(MAKE) version="$(VERSION)" -f geopm.spec.mk $@

if ENABLE_DOC

# DOXYGEN TARGET
doxygen:
	doxygen dox/Doxyfile

# MAN TARGET
BUILT_SOURCES = ruby/bin/ronn

geopm-tutorial.tar.gz: $(TUTORIAL_DIST)
	tar --transform='s|^|geopm-|' -zcf $@ $^

ruby/bin/ronn:
	mkdir -p ruby/bin; \
	which ronn >& /dev/null && \
	ln -sf `which ronn` ruby/bin/ronn || \
	gem install ronn -i ruby
	if [ ! -f $@ ]; then pushd ruby/bin && ln -sf ronn.ruby* ronn && popd; fi

$(ronn_man): man/%: ruby/bin/ronn ronn/%.ronn ronn/index.txt ronn/header.txt
	mkdir -p man
	export GEM_PATH=`readlink -f $(top_srcdir)/ruby`:$$GEM_PATH && \
	cd ronn && \
	../ruby/bin/ronn --date=`date "+%Y-%m-%d"` \
	                 --organization="Intel Corporation" \
	                 `basename $@`.ronn
	cat ronn/header.txt ronn/`basename $@` | \
	    sed -e 's/\(#include.*\) \\fIhttp.*/\1\\fR/g' > $@

# GH_PAGES TARGET
gh-pages: doxygen $(dist_man_MANS)
	mkdir -p gh-pages/dox
	mkdir -p gh-pages/man
	cp -rf dox/html/* gh-pages/dox/.
	cp ronn/*.html gh-pages/man/.

else

BUILT_SOURCES =

$(ronn_man): man/%: ronn/%.ronn
	mkdir -p man
	sed 's|^\[//\]: #|.\\\"|' ronn/`basename $@`.ronn > $@
endif

man/geopmsrun.1: man/geopmpy_launcher.1
	cp $^ $@
	cp ronn/geopmpy_launcher.1.html ronn/geopmsrun.1.html

man/geopmaprun.1: man/geopmpy_launcher.1
	cp $^ $@
	cp ronn/geopmpy_launcher.1.html ronn/geopmsrun.1.html

checkprogs: fortran-checkprogs gtest-checkprogs pytest-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)

# INCLUDES
check_PROGRAMS =
check_LTLIBRARIES =
noinst_PROGRAMS =
noinst_LTLIBRARIES =
TESTS =

PHONY_TARGETS = clean-local \
                clean-local-coverage \
                clean-local-man \
                distclean-local \
                rpm \
                doxygen \
                gh_pages \
                #end

include test/Makefile.mk
include test_integration/Makefile.mk
include examples/Makefile.mk
include plugin/Makefile.mk
include doc/Makefile.mk
include scripts/Makefile.mk
include openmp.mk

.PHONY: $(PHONY_TARGETS)
