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
               -D_POSIX_C_SOURCE=200809L \
               -DOMPI_SKIP_MPICXX

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopmpolicy.la
bin_PROGRAMS = geopmadmin \
               geopmagent \
               #end
pkglib_LTLIBRARIES =
nodist_include_HEADERS =

if ENABLE_MPI
    lib_LTLIBRARIES += libgeopm.la
    bin_PROGRAMS += geopmctl \
                    # end
if ENABLE_OPENMP
    bin_PROGRAMS += geopmbench \
                    # end
endif
if ENABLE_FORTRAN
    lib_LTLIBRARIES += libgeopmfort.la
endif
endif

install-exec-hook: install-python install-fortran install-plugin-dir
dist: dist-googletest


include_HEADERS = src/geopm_agent.h \
                  src/geopm_ctl.h \
                  src/geopm_imbalancer.h \
                  # end
if ENABLE_BETA
    include_HEADERS += src/geopm_daemon.h \
                       src/geopm_endpoint.h \
                       src/geopm_policystore.h \
                       # end
endif

pkginclude_HEADERS = src/Agent.hpp \
                     src/EnergyEfficientRegion.hpp \
                     src/FrequencyGovernor.hpp \
                     src/PlatformIOProf.hpp \
                     src/PowerBalancer.hpp \
                     src/PowerGovernor.hpp \
                     src/SampleAggregator.hpp \
                     # end

if ENABLE_BETA
    pkginclude_HEADERS += src/Daemon.hpp \
                          src/Endpoint.hpp \
                          # end
endif

all_local = $(roff_man_noinst)

if ENABLE_MPI
if ENABLE_FORTRAN

all_local += geopm.mod

install-fortran:
	$(INSTALL) -d $(DESTDIR)/$(libdir)/$(FC)/modules/geopm-`uname -m`
	$(INSTALL) geopm.mod $(DESTDIR)/$(libdir)/$(FC)/modules/geopm-`uname -m`
else

install-fortran:
	-
endif
else
install-fortran:
	-
endif

all-local: $(all_local)

install-plugin-dir:
	$(INSTALL) -d $(DESTDIR)/$(pkglibdir)

# The transform variable will transform any installed file beginning
# with "GEOPM_CXX_MAN_" to begin with "geopm::".  This is to avoid
# having make targets that contain the colon character.  See autotools
# documentation below.
#
# https://www.gnu.org/software/autoconf/manual/autoconf-2.67/html_node/Transformation-Rules.html
#
transform='s/GEOPM_CXX_MAN_/geopm::/'

roff_man_inst = man/GEOPM_CXX_MAN_Agent.3 \
                man/GEOPM_CXX_MAN_EnergyEfficientRegion.3 \
                man/GEOPM_CXX_MAN_PowerBalancer.3 \
                man/GEOPM_CXX_MAN_PowerGovernor.3 \
                man/geopm_agent_c.3 \
                man/geopm_agent_energy_efficient.7 \
                man/geopm_agent_frequency_map.7 \
                man/geopm_agent_monitor.7 \
                man/geopm_agent_power_balancer.7 \
                man/geopm_agent_power_governor.7 \
                man/geopm_ctl_c.3 \
                man/geopm_fortran.3 \
                man/geopm_imbalancer.3 \
                man/geopm_prof_c.3 \
                man/geopmadmin.1 \
                man/geopmagent.1 \
                man/geopmbench.1 \
                man/geopmctl.1 \
                man/geopmlaunch.1 \
                man/geopmpy.7 \
                # end

roff_man_noinst = man/GEOPM_CXX_MAN_Comm.3 \
                  man/GEOPM_CXX_MAN_EnergyEfficientAgent.3 \
                  man/GEOPM_CXX_MAN_MonitorAgent.3 \
                  man/GEOPM_CXX_MAN_MPIComm.3 \
                  man/GEOPM_CXX_MAN_PowerBalancerAgent.3 \
                  man/GEOPM_CXX_MAN_PowerGovernorAgent.3 \
                  man/GEOPM_CXX_MAN_ProfileIOGroup.3 \
                  # end

roff_man_beta = man/geopmendpoint.1 \
                man/geopmplotter.1 \
                man/geopm_daemon_c.3 \
                man/geopm_endpoint_c.3 \
                man/geopm_policystore_c.3 \
                man/GEOPM_CXX_MAN_Daemon.3 \
                man/GEOPM_CXX_MAN_Endpoint.3 \
                # end

if ENABLE_BETA
roff_man_inst += $(roff_man_beta)
else
roff_man_noinst += $(roff_man_beta)
endif

roff_man = $(roff_man_inst) $(roff_man_noinst)

dist_man_MANS = $(roff_man_inst) \
                # end

dist_doc_DATA = COPYING \
                README \
                VERSION \
                # end

EXTRA_DIST = $(TUTORIAL_DIST) \
             .github/include_guards.sh \
             .github/workflows/build.yml \
             .github/ISSUE_TEMPLATE/bug_report.md \
             .github/ISSUE_TEMPLATE/feature_request.md \
             .github/ISSUE_TEMPLATE/story.md \
             .github/ISSUE_TEMPLATE/change_bug_fix.md \
             .github/ISSUE_TEMPLATE/change_build.md \
             .github/ISSUE_TEMPLATE/change_complete_feature.md \
             .github/ISSUE_TEMPLATE/change_cxx_add_class.md \
             .github/ISSUE_TEMPLATE/change_cxx_add_methods.md \
             .github/ISSUE_TEMPLATE/change_doc.md \
             .github/ISSUE_TEMPLATE/change_ext_dep.md \
             .github/ISSUE_TEMPLATE/change_imp.md \
             .github/ISSUE_TEMPLATE/change.md \
             .github/ISSUE_TEMPLATE/change_python_add_class.md \
             .github/ISSUE_TEMPLATE/change_python_add_methods.md \
             .github/ISSUE_TEMPLATE/change_remove_imp.md \
             .github/ISSUE_TEMPLATE/change_test.md \
             .github/ISSUE_TEMPLATE/tech_debt.md \
             .github/ISSUE_TEMPLATE/bug_report.md \
             .github/ISSUE_TEMPLATE/feature_request.md \
             .github/ISSUE_TEMPLATE/story.md \
             .github/ISSUE_TEMPLATE/change.md \
             .github/ISSUE_TEMPLATE/tech_debt.md \
             .gitignore \
             BLURB \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.rst \
             ChangeLog \
             MANIFEST \
             Makefile.am \
             README.md \
             TODO \
             autogen.sh \
             configure.ac \
             copying_headers/MANIFEST.BSD3-llnl \
             copying_headers/MANIFEST.EXEMPT \
             copying_headers/MANIFEST.MIT-dropbox \
             copying_headers/MANIFEST.NAS \
             copying_headers/header.BSD3-intel \
             copying_headers/header.BSD3-llnl \
             copying_headers/header.MIT-dropbox \
             copying_headers/header.NAS \
             copying_headers/test-dist \
             copying_headers/test-license \
             dox/Doxyfile.in \
             gen_man.sh \
             geopm-tutorial.tar.gz \
             indicators/index.html \
             indicators/indicators.py \
             pull_request_template.md \
             scripts/outlier/README.md \
             scripts/outlier/outlier_detection.py \
             scripts/outlier/theta_nodelist_broken.txt \
             specs/geopm.spec \
             specs/geopm.spec.in \
             specs/geopm-dudley.spec \
             specs/geopm-dudley.spec.in \
             specs/geopm-gcc-mvapich2-toss.spec \
             specs/geopm-intel-mvapich2-toss.spec \
             specs/geopm-ohpc.spec \
             specs/geopm-ohpc.spec.in \
             specs/geopm-pyyaml-req.patch \
             specs/geopm-theta.spec \
             specs/geopm-theta.spec.in \
             test_hw/Makefile \
             test_hw/geopm_time.h \
             test_hw/rapl_pkg_limit_plot.py \
             test_hw/rapl_pkg_limit_sweep.sh \
             test_hw/rapl_pkg_limit_test.c \
             test_license.sh \
             # end

TUTORIAL_DIST = tutorial/Makefile \
                tutorial/README.md \
                tutorial/admin/README.md \
                tutorial/admin/00_check_allowlist.sh \
                tutorial/admin/00_test_prereqs.sh \
                tutorial/admin/03_setup_fixed_frequency.sh \
                tutorial/admin/04_setup_default_energy_efficient.sh \
                tutorial/admin/05_setup_fixed_power_cap.sh \
                tutorial/admin/06_setup_default_power_balancer.sh \
                tutorial/admin/07_setup_override_energy_efficient.sh \
                tutorial/admin/08_setup_override_power_balancer.sh \
                tutorial/agent/ExampleAgent.cpp \
                tutorial/agent/ExampleAgent.hpp \
                tutorial/agent/README.md \
                tutorial/agent/agent_tutorial.sh \
                tutorial/agent/agent_tutorial_config.json \
                tutorial/agent/example_policy.json \
                tutorial/iogroup/ExampleIOGroup.cpp \
                tutorial/iogroup/ExampleIOGroup.hpp \
                tutorial/iogroup/README.md \
                tutorial/plugin_load/alice/AliceIOGroup.cpp \
                tutorial/plugin_load/alice/AliceIOGroup.hpp \
                tutorial/plugin_load/bob/BobIOGroup.cpp \
                tutorial/plugin_load/bob/BobIOGroup.hpp \
                tutorial/plugin_load/README.md \
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
                tutorial/tutorial_6.sh \
                tutorial/tutorial_6_config.json \
                tutorial/tutorial_build_gnu.sh \
                tutorial/tutorial_build_intel.sh \
                tutorial/tutorial_env.sh \
                tutorial/tutorial_power_policy.json \
                tutorial/tutorial_region.c \
                tutorial/tutorial_region_prof.c \
                tutorial/tutorial_region.h \
                # end

if HAVE_ICC
    MATH_CLDFLAGS = -lm -mkl
    MATH_CFLAGS = -xAVX
    MATH_LIB = -lm
else
    MATH_CLDFLAGS = -mavx
    MATH_CFLAGS = -mavx
    MATH_LIB = -lm
endif

# EXTEND FLAGS FOR MPI TARGETS
if ENABLE_MPI
    MPI_CFLAGS += -DGEOPM_ENABLE_MPI
    MPI_FCFLAGS += -DGEOPM_ENABLE_MPI
    libgeopm_la_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) -version-info $(geopm_abi_version)
    libgeopm_la_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    libgeopm_la_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS)

    geopmctl_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS)
    geopmctl_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    geopmctl_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS)

    geopmbench_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) $(MATH_CLDFLAGS)
    geopmbench_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c99 $(MATH_CFLAGS)
    geopmbench_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CFLAGS) -D_GNU_SOURCE -std=c++11 $(MATH_CFLAGS)
if ENABLE_FORTRAN
    libgeopmfort_la_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
    libgeopmfort_la_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    libgeopmfort_la_LDFLAGS = $(AM_LDFLAGS) $(MPI_CLDFLAGS) -version-info $(geopm_abi_version)
endif
endif

# EXTEND FLAGS FOR NON-MPI TARGETS
libgeopmpolicy_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)

# ADD LIBRARY DEPENDENCIES FOR EXECUTABLES
geopmagent_LDADD = libgeopmpolicy.la
geopmadmin_LDADD = libgeopmpolicy.la
if ENABLE_MPI
    geopmctl_LDADD = libgeopm.la $(MPI_CLIBS)
    geopmbench_LDADD = libgeopm.la $(MATH_LIB) $(MPI_CLIBS)
    libgeopm_la_LIBADD = $(MPI_CLIBS)
if ENABLE_FORTRAN
    libgeopmfort_la_LIBADD = libgeopm.la $(MPI_FLIBS)
endif
endif

libgeopmpolicy_la_SOURCES = src/Accumulator.cpp \
                            src/Accumulator.hpp \
                            src/Admin.cpp \
                            src/Admin.hpp \
                            src/Agent.cpp \
                            src/Agent.hpp \
                            src/ApplicationIO.cpp \
                            src/ApplicationIO.hpp \
                            src/ApplicationRecordLog.cpp \
                            src/ApplicationRecordLog.hpp \
                            src/ApplicationSampler.cpp \
                            src/ApplicationSampler.hpp \
                            src/ApplicationSamplerImp.hpp \
                            src/ApplicationStatus.cpp \
                            src/ApplicationStatus.hpp \
                            src/Comm.cpp \
                            src/Comm.hpp \
                            src/ControlMessage.cpp \
                            src/ControlMessage.hpp \
                            src/Controller.cpp \
                            src/Controller.hpp \
                            src/CSV.cpp \
                            src/CSV.hpp \
                            src/DebugIOGroup.cpp \
                            src/DebugIOGroup.hpp \
                            src/EditDistPeriodicityDetector.cpp \
                            src/EditDistPeriodicityDetector.hpp \
                            src/EditDistEpochRecordFilter.cpp \
                            src/EditDistEpochRecordFilter.hpp \
                            src/ELF.cpp \
                            src/ELF.hpp \
                            src/Endpoint.cpp \
                            src/Endpoint.hpp \
                            src/EndpointImp.hpp \
                            src/EndpointPolicyTracer.cpp \
                            src/EndpointPolicyTracer.hpp \
                            src/EndpointPolicyTracerImp.hpp \
                            src/EndpointUser.cpp \
                            src/EndpointUser.hpp \
                            src/EnergyEfficientAgent.cpp \
                            src/EnergyEfficientAgent.hpp \
                            src/EnergyEfficientRegion.cpp \
                            src/EnergyEfficientRegion.hpp \
                            src/Environment.cpp \
                            src/Environment.hpp \
                            src/EpochIOGroup.cpp \
                            src/EpochIOGroup.hpp \
                            src/FilePolicy.cpp \
                            src/FilePolicy.hpp \
                            src/FrequencyGovernor.cpp \
                            src/FrequencyGovernor.hpp \
                            src/FrequencyGovernorImp.hpp \
                            src/FrequencyMapAgent.cpp \
                            src/FrequencyMapAgent.hpp \
                            src/Imbalancer.cpp \
                            src/ModelParse.cpp \
                            src/ModelParse.hpp \
                            src/MonitorAgent.cpp \
                            src/MonitorAgent.hpp \
                            src/OptionParser.cpp \
                            src/OptionParser.hpp \
                            src/PowerBalancer.cpp \
                            src/PowerBalancer.hpp \
                            src/PowerBalancerImp.hpp \
                            src/PowerBalancerAgent.cpp \
                            src/PowerBalancerAgent.hpp \
                            src/PowerGovernor.cpp \
                            src/PowerGovernor.hpp \
                            src/PowerGovernorImp.hpp \
                            src/PowerGovernorAgent.cpp \
                            src/PowerGovernorAgent.hpp \
                            src/PlatformIOProf.cpp \
                            src/PlatformIOProf.hpp \
                            src/ProfileIOGroup.cpp \
                            src/ProfileIOGroup.hpp \
                            src/ProfileSampler.cpp \
                            src/ProfileSampler.hpp \
                            src/ProfileTable.cpp \
                            src/ProfileTable.hpp \
                            src/ProfileTracer.cpp \
                            src/ProfileTracer.hpp \
                            src/ProfileTracerImp.hpp \
                            src/ProxyEpochRecordFilter.cpp \
                            src/ProxyEpochRecordFilter.hpp \
                            src/ProcessRegionAggregator.cpp \
                            src/ProcessRegionAggregator.hpp \
                            src/RecordFilter.cpp \
                            src/RecordFilter.hpp \
                            src/Reporter.cpp \
                            src/Reporter.hpp \
                            src/SampleAggregator.cpp \
                            src/SampleAggregator.hpp \
                            src/SampleAggregatorImp.hpp \
                            src/Scheduler.cpp \
                            src/Scheduler.hpp \
                            src/Tracer.cpp \
                            src/Tracer.hpp \
                            src/TreeComm.cpp \
                            src/TreeComm.hpp \
                            src/TreeCommLevel.cpp \
                            src/TreeCommLevel.hpp \
                            src/ValidateRecord.cpp \
                            src/ValidateRecord.hpp \
                            src/geopm_agent.h \
                            src/geopm_endpoint.h \
                            src/geopm_imbalancer.h \
                            src/geopm_policystore.h \
                            src/geopm_reporter.h \
                            src/record.cpp \
                            src/record.hpp \
                            # end

beta_source_files = src/Daemon.cpp \
                    src/Daemon.hpp \
                    src/DaemonImp.hpp \
                    src/PolicyStore.cpp \
                    src/PolicyStore.hpp \
                    src/PolicyStoreImp.cpp \
                    src/PolicyStoreImp.hpp \
                    #end

if ENABLE_BETA
    libgeopmpolicy_la_SOURCES += $(beta_source_files)
else
    EXTRA_DIST += $(beta_source_files)
endif

geopmagent_SOURCES = src/geopmagent_main.cpp
geopmadmin_SOURCES = src/geopmadmin_main.cpp

mpi_source_files = src/DefaultProfile.cpp \
                   src/MPIComm.cpp \
                   src/MPIComm.hpp \
                   src/MPICommSplit.cpp \
                   src/Profile.cpp \
                   src/Profile.hpp \
                   src/geopm_ctl.h \
                   src/geopm_mpi_comm_split.h \
                   src/geopm_pmpi.c \
                   src/geopm_pmpi_helper.cpp \
                   src/geopm_pmpi.h \
                   # end
ompt_source_files = src/OMPT.cpp \
                    src/OMPT.hpp \
                    src/ompt_callback.cpp \
                    # end
fortran_source_files = src/geopm.f90 \
                       src/geopm_pmpi_fortran.c \
                       # end
geopmctl_source_files = src/geopmctl_main.c \
                        # end

if ENABLE_MPI
    # All source files that are compiled into libgeopmpolicy are also
    # compiled into libgeopm.  We either have to do this or require
    # users to link to both libgeopm and libgeopmpolicy when calling
    # into application facing APIs.
    libgeopm_la_SOURCES = $(libgeopmpolicy_la_SOURCES) \
                          $(mpi_source_files) \
                          # end
    geopmctl_SOURCES = $(geopmctl_source_files)

if ENABLE_OMPT
    libgeopm_la_SOURCES += $(ompt_source_files)
else
    EXTRA_DIST += $(ompt_source_files)
endif

if ENABLE_FORTRAN
    libgeopmfort_la_SOURCES = $(fortran_source_files)
else
    EXTRA_DIST += $(fortran_source_files)
endif

# disable MPI
else
    EXTRA_DIST += $(mpi_source_files) \
                  $(ompt_source_files) \
                  $(fortran_source_files) \
                  $(geopmctl_source_files) \
                  # end
endif

model_source_files = src/All2allModelRegion.cpp \
                     src/All2allModelRegion.hpp \
                     src/BarrierModelRegion.cpp \
                     src/BarrierModelRegion.hpp \
                     src/DGEMMModelRegion.cpp \
                     src/DGEMMModelRegion.hpp \
                     src/IgnoreModelRegion.cpp \
                     src/IgnoreModelRegion.hpp \
                     src/ModelApplication.cpp \
                     src/ModelApplication.hpp \
                     src/ModelRegion.cpp \
                     src/ModelRegion.hpp \
                     src/NestedModelRegion.cpp \
                     src/NestedModelRegion.hpp \
                     src/ReduceModelRegion.cpp \
                     src/ReduceModelRegion.hpp \
                     src/ScalingModelRegion.cpp \
                     src/ScalingModelRegion.hpp \
                     src/SleepModelRegion.cpp \
                     src/SleepModelRegion.hpp \
                     src/SpinModelRegion.cpp \
                     src/SpinModelRegion.hpp \
                     src/StreamModelRegion.cpp \
                     src/StreamModelRegion.hpp \
                     src/TimedScalingModelRegion.cpp \
                     src/TimedScalingModelRegion.hpp \
                     # end

if ENABLE_MPI
if ENABLE_OPENMP
    geopmbench_SOURCES = $(model_source_files) \
                         src/geopmbench_main.cpp \
                         # end
else
    EXTRA_DIST += $(model_source_files) \
                  src/geopmbench_main.cpp \
                  # end
endif
else
    EXTRA_DIST += $(model_source_files) \
                  src/geopmbench_main.cpp \
                  # end
endif

# CLEAN TARGETS
DISTCLEANFILES = VERSION MANIFEST

CLEAN_LOCAL_TARGETS= clean-local-coverage \
                     clean-local-fortran \
                     clean-local-googletest \
                     clean-local-man \
                     # end

clean-local: $(CLEAN_LOCAL_TARGETS)

distclean-local: distclean-local-ruby

clean-local-coverage:
	rm -f `find -name \*.gcno` `find -name \*.gcda`

clean-local-man:
	rm -rf man

clean-local-fortran:
	rm -rf geopm.mod

distclean-local-ruby:
	rm -rf ruby

# FORTRAN MODULE TARGET
if ENABLE_MPI
if ENABLE_FORTRAN
BUILT_SOURCES = geopm.mod
geopm.mod: src/geopm.f90
	$(FC) $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS) -c $<
else
BUILT_SOURCES =
endif
else
BUILT_SOURCES =
endif

# RPM TARGET
rpm_topdir ?= $(HOME)/rpmbuild
rpm: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-$(VERSION).tar.gz $(rpm_topdir)/SOURCES/geopm.tar.gz
	cp specs/geopm.spec $(rpm_topdir)/SPECS/geopm.spec
	rpmbuild $(rpmbuild_flags) -ba $(rpm_topdir)/SPECS/geopm.spec

rpm-theta: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-$(VERSION).tar.gz $(rpm_topdir)/SOURCES/
	cp specs/geopm-theta.spec $(rpm_topdir)/SPECS/geopm-$(VERSION).spec
	rpmbuild $(rpmbuild_flags) --buildroot=$(rpm_topdir)-root --define "_sourcedir $(rpm_topdir)/SOURCES" -ba $(rpm_topdir)/SPECS/geopm-$(VERSION).spec

rpm-dudley: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-$(VERSION).tar.gz $(rpm_topdir)/SOURCES/
	cp specs/geopm-dudley.spec $(rpm_topdir)/SPECS/geopm-$(VERSION).spec
	cp specs/geopm-pyyaml-req.patch $(rpm_topdir)/SOURCES/
	rpmbuild $(rpmbuild_flags) --buildroot=$(rpm_topdir)-root --define "_sourcedir $(rpm_topdir)/SOURCES" -ba $(rpm_topdir)/SPECS/geopm-$(VERSION).spec

# DOXYGEN TARGETS
dox-public/Doxyfile: $(top_srcdir)/dox/Doxyfile.in
	mkdir -p dox-public
	sed -e "s|@DOX_VERSION@|$(VERSION)|g" \
	    -e "s|@DOX_OUTPUT@|$(abs_builddir)/dox-public|g" \
	    -e "s|@DOX_INPUT@|README.md $(include_HEADERS) $(pkginclude_HEADERS)|g" \
        $^ > $@

dox/Doxyfile: $(top_srcdir)/dox/Doxyfile.in
	mkdir -p dox
	sed -e "s|@DOX_VERSION@|$(VERSION)|g" \
	    -e "s|@DOX_OUTPUT@|$(abs_builddir)/dox|g" \
	    -e "s|@DOX_INPUT@|README.md src|g" \
        $^ > $@

doxygen: dox-public/Doxyfile dox/Doxyfile
	cd $(top_srcdir) && doxygen $(abs_builddir)/dox/Doxyfile
	cd $(top_srcdir) && doxygen $(abs_builddir)/dox-public/Doxyfile

geopm-tutorial.tar.gz: $(TUTORIAL_DIST)
	tar --transform='s|^|geopm-|' -zcf $@ $^

checkprogs: gtest-checkprogs pytest-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)

# INCLUDES
check_PROGRAMS =
check_LTLIBRARIES =
noinst_PROGRAMS =
noinst_LTLIBRARIES =
TESTS = test_license.sh

PHONY_TARGETS = clean-local \
                clean-local-coverage \
                clean-local-man \
                distclean-local \
                doxygen \
                gh_pages \
                rpm \
                # end

include test/Makefile.mk
include integration/Makefile.mk
include examples/Makefile.mk
include scripts/Makefile.mk
include shell_completion/Makefile.mk

.PHONY: $(PHONY_TARGETS)
