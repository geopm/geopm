#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# GLOBAL SETTINGS
ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS += -I$(top_srcdir)/src \
               -DGEOPM_DEFAULT_PLUGIN_PATH=\"$(libdir)/geopm\" \
               -D_POSIX_C_SOURCE=200809L \
               -DOMPI_SKIP_MPICXX \
               # end

# THINGS THAT ARE INSTALLED
lib_LTLIBRARIES = libgeopm.la \
                  # end

bin_PROGRAMS = geopmadmin \
               geopmagent \
               geopmctl \
               #end

if ENABLE_OPENMP
    bin_PROGRAMS += geopmbench
endif
if ENABLE_FORTRAN
    lib_LTLIBRARIES += libgeopmfort.la
endif

pkglib_LTLIBRARIES =
nodist_include_HEADERS =

install-exec-hook: install-python install-fortran
dist: dist-googletest


include_HEADERS = src/geopm_agent.h \
                  src/geopm_imbalancer.h \
                  src/geopm_prof.h \
                  # end
if ENABLE_BETA
    include_HEADERS += src/geopm_daemon.h \
                       src/geopm_endpoint.h \
                       src/geopm_policystore.h \
                       # end
endif

geopmincludedir = $(includedir)/geopm

geopminclude_HEADERS = src/Agent.hpp \
                       src/FrequencyGovernor.hpp \
                       src/PlatformIOProf.hpp \
                       src/PowerBalancer.hpp \
                       src/PowerGovernor.hpp \
                       src/SampleAggregator.hpp \
                       # end

if ENABLE_BETA
    geopminclude_HEADERS += src/Daemon.hpp \
                            src/Endpoint.hpp \
                            # end
endif

all_local = $(roff_man_noinst)

if ENABLE_MPI
if ENABLE_FORTRAN

all_local += geopm_prof.mod

install-fortran:
	$(INSTALL) -d $(DESTDIR)/$(libdir)/$(FC)/modules/geopm-`uname -m`
	$(INSTALL) geopm_prof.mod $(DESTDIR)/$(libdir)/$(FC)/modules/geopm-`uname -m`
else

install-fortran:
	-
endif
else
install-fortran:
	-
endif # ENABLE_MPI

all-local: $(all_local)

roff_man_inst = docs/build/man/GEOPM_CXX_MAN_Agent.3 \
                docs/build/man/GEOPM_CXX_MAN_PowerBalancer.3 \
                docs/build/man/GEOPM_CXX_MAN_PowerGovernor.3 \
                docs/build/man/geopm_agent.3 \
                docs/build/man/geopm_agent_frequency_map.7 \
                docs/build/man/geopm_agent_monitor.7 \
                docs/build/man/geopm_agent_power_balancer.7 \
                docs/build/man/geopm_agent_power_governor.7 \
                docs/build/man/geopm_ctl.3 \
                docs/build/man/geopm_fortran.3 \
                docs/build/man/geopm_imbalancer.3 \
                docs/build/man/geopm_prof.3 \
                docs/build/man/geopmadmin.1 \
                docs/build/man/geopmagent.1 \
                docs/build/man/geopmbench.1 \
                docs/build/man/geopmctl.1 \
                docs/build/man/geopmlaunch.1 \
                docs/build/man/geopmpy.7 \
                # end

roff_man_noinst = docs/build/man/GEOPM_CXX_MAN_Comm.3 \
                  docs/build/man/GEOPM_CXX_MAN_MonitorAgent.3 \
                  docs/build/man/GEOPM_CXX_MAN_MPIComm.3 \
                  docs/build/man/GEOPM_CXX_MAN_PowerBalancerAgent.3 \
                  docs/build/man/GEOPM_CXX_MAN_PowerGovernorAgent.3 \
                  docs/build/man/GEOPM_CXX_MAN_ProfileIOGroup.3 \
                  # end

roff_man_beta = docs/build/man/geopmendpoint.1 \
                docs/build/man/geopm_agent_cpu_activity.7 \
                docs/build/man/geopm_agent_gpu_activity.7 \
                docs/build/man/geopm_daemon.3 \
                docs/build/man/geopm_endpoint.3 \
                docs/build/man/geopm_policystore.3 \
                docs/build/man/GEOPM_CXX_MAN_Daemon.3 \
                docs/build/man/GEOPM_CXX_MAN_Endpoint.3 \
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
                COPYING-TPP \
                README \
                VERSION \
                # end

EXTRA_DIST = $(TUTORIAL_DIST) \
             .codespellrc \
             .github/include_guards.sh \
             .github/workflows/build.yml \
             .github/workflows/codeql-analysis.yml \
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
             .pre-commit-config.yaml \
             CODE_OF_CONDUCT.md \
             CONTRIBUTING.rst \
             ChangeLog \
             MANIFEST \
             Makefile.am \
             README.md \
             SECURITY.md \
             TODO \
             autogen.sh \
             configure.ac \
             copying_headers/MANIFEST.apache2-llvm \
             copying_headers/MANIFEST.autoconf_macro \
             copying_headers/MANIFEST.BSD3-llnl \
             copying_headers/MANIFEST.compile_flag \
             copying_headers/MANIFEST.EXEMPT \
             copying_headers/MANIFEST.MIT-dropbox \
             copying_headers/MANIFEST.NAS \
             copying_headers/header.apache2-llvm \
             copying_headers/header.autoconf_macro \
             copying_headers/header.BSD3-intel \
             copying_headers/header.BSD3-llnl \
             copying_headers/header.compile_flag \
             copying_headers/header.MIT-dropbox \
             copying_headers/header.NAS \
             copying_headers/test-dist \
             copying_headers/test-license \
             debian/changelog \
             debian/changelog.in \
             debian/control \
             debian/copyright \
             debian/geopm-runtime.dirs \
             debian/geopm-runtime.install \
             debian/libgeopm-dev.dirs \
             debian/libgeopm-dev.install \
             debian/libgeopm2.install \
             debian/python3-geopmpy.install \
             debian/rules \
             dox/Doxyfile.in \
             doxygen_update.sh \
             geopm-tutorial.tar.gz \
             indicators/index.html \
             indicators/indicators.py \
             m4/ax_check_compile_flag.m4 \
             m4/openmp.m4 \
             pull_request_template.md \
             geopm-runtime.spec \
             geopm-runtime.spec.in \
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
                tutorial/admin/04_setup_default_frequency_map.sh \
                tutorial/admin/05_setup_fixed_power_cap.sh \
                tutorial/admin/06_setup_default_power_balancer.sh \
                tutorial/admin/07_setup_override_power_balancer.sh \
                tutorial/agent/ExampleAgent.cpp \
                tutorial/agent/ExampleAgent.hpp \
                tutorial/agent/README.md \
                tutorial/agent/agent_tutorial.sh \
                tutorial/agent/agent_tutorial_config.json \
                tutorial/agent/example_policy.json \
                tutorial/iogroup/ExampleIOGroup.cpp \
                tutorial/iogroup/ExampleIOGroup.hpp \
                tutorial/iogroup/README.md \
                tutorial/local/README.md \
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

if HAVE_ICX
    MATH_CLDFLAGS = -lm -qmkl
    MATH_CFLAGS = -xAVX
    MATH_LIB = -lm
else
    MATH_CLDFLAGS = -mavx
    MATH_CFLAGS = -mavx
    MATH_LIB = -lm
endif

geopmbench_LDFLAGS = $(AM_LDFLAGS) $(MATH_CLDFLAGS)
geopmbench_CFLAGS = $(AM_CFLAGS) -D_GNU_SOURCE -std=c99 $(MATH_CFLAGS)
geopmbench_CXXFLAGS = $(AM_CXXFLAGS) -D_GNU_SOURCE -std=c++17 $(MATH_CFLAGS)

# ADD LIBRARY DEPENDENCIES FOR EXECUTABLES
geopmagent_LDADD = libgeopm.la
geopmadmin_LDADD = libgeopm.la
geopmctl_LDADD = libgeopm.la
geopmbench_LDADD = libgeopm.la $(MATH_LIB)
if ENABLE_MPI
geopmctl_LDADD += $(MPI_CLIBS)
geopmbench_LDADD += $(MPI_CLIBS)
libgeopm_la_LIBADD = $(MPI_CLIBS)
endif


libgeopm_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)
libgeopm_la_CFLAGS = $(AM_CFLAGS)
libgeopm_la_CXXFLAGS = $(AM_CXXFLAGS)

libgeopm_la_SOURCES = src/Accumulator.cpp \
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
                      src/Controller.cpp \
                      src/Controller.hpp \
                      src/CSV.cpp \
                      src/CSV.hpp \
                      src/DebugIOGroup.cpp \
                      src/DebugIOGroup.hpp \
                      src/DefaultProfile.cpp \
                      src/DenseLayer.cpp \
                      src/DenseLayer.hpp \
                      src/DenseLayerImp.hpp \
                      src/DomainNetMap.cpp \
                      src/DomainNetMap.hpp \
                      src/DomainNetMapImp.hpp \
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
                      src/Environment.cpp \
                      src/Environment.hpp \
                      src/EnvironmentParser.cpp \
                      src/EnvironmentParser.hpp \
                      src/EpochIOGroup.cpp \
                      src/EpochIOGroup.hpp \
                      src/FilePolicy.cpp \
                      src/FilePolicy.hpp \
                      src/FrequencyGovernor.cpp \
                      src/FrequencyGovernor.hpp \
                      src/FrequencyGovernorImp.hpp \
                      src/FrequencyLimitDetector.cpp \
                      src/FrequencyLimitDetector.hpp \
                      src/SSTFrequencyLimitDetector.cpp \
                      src/SSTFrequencyLimitDetector.hpp \
                      src/TRLFrequencyLimitDetector.cpp \
                      src/TRLFrequencyLimitDetector.hpp \
                      src/FrequencyMapAgent.cpp \
                      src/FrequencyMapAgent.hpp \
                      src/FrequencyTimeBalancer.cpp \
                      src/FrequencyTimeBalancer.hpp \
                      src/Imbalancer.cpp \
                      src/InitControl.cpp \
                      src/InitControl.hpp \
                      src/LocalNeuralNet.cpp \
                      src/LocalNeuralNet.hpp \
                      src/LocalNeuralNetImp.hpp \
                      src/ModelParse.cpp \
                      src/ModelParse.hpp \
                      src/MonitorAgent.cpp \
                      src/MonitorAgent.hpp \
                      src/NNFactory.cpp \
                      src/NNFactory.hpp \
                      src/NNFactoryImp.hpp \
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
                      src/Profile.cpp \
                      src/Profile.hpp \
                      src/ProfileIOGroup.cpp \
                      src/ProfileIOGroup.hpp \
                      src/ProfileTracer.cpp \
                      src/ProfileTracer.hpp \
                      src/ProfileTracerImp.hpp \
                      src/ProxyEpochRecordFilter.cpp \
                      src/ProxyEpochRecordFilter.hpp \
                      src/ProcessRegionAggregator.cpp \
                      src/ProcessRegionAggregator.hpp \
                      src/RecordFilter.cpp \
                      src/RecordFilter.hpp \
                      src/RegionHintRecommender.cpp \
                      src/RegionHintRecommender.hpp \
                      src/RegionHintRecommenderImp.hpp \
                      src/Reporter.cpp \
                      src/Reporter.hpp \
                      src/SampleAggregator.cpp \
                      src/SampleAggregator.hpp \
                      src/SampleAggregatorImp.hpp \
                      src/Scheduler.cpp \
                      src/Scheduler.hpp \
                      src/SSTClosGovernor.cpp \
                      src/SSTClosGovernor.hpp \
                      src/SSTClosGovernorImp.hpp \
                      src/TensorMath.cpp \
                      src/TensorMath.hpp \
                      src/TensorOneD.cpp \
                      src/TensorOneD.hpp \
                      src/TensorTwoD.cpp \
                      src/TensorTwoD.hpp \
                      src/Tracer.cpp \
                      src/Tracer.hpp \
                      src/TreeComm.cpp \
                      src/TreeComm.hpp \
                      src/TreeCommLevel.cpp \
                      src/TreeCommLevel.hpp \
                      src/ValidateRecord.cpp \
                      src/ValidateRecord.hpp \
                      src/Waiter.cpp \
                      src/Waiter.hpp \
                      src/geopm_agent.h \
                      src/geopm_daemon.h \
                      src/geopm_endpoint.h \
                      src/geopm_imbalancer.h \
                      src/geopm_lib_init.cpp \
                      src/geopm_policystore.h \
                      src/geopm_prof.h \
                      src/record.cpp \
                      src/record.hpp \
                      # end

beta_source_files = src/CPUActivityAgent.cpp \
                    src/CPUActivityAgent.hpp \
                    src/Daemon.cpp \
                    src/Daemon.hpp \
                    src/DaemonImp.hpp \
                    src/FFNetAgent.cpp \
                    src/FFNetAgent.hpp \
                    src/GPUActivityAgent.cpp \
                    src/GPUActivityAgent.hpp \
                    src/PolicyStore.cpp \
                    src/PolicyStore.hpp \
                    src/PolicyStoreImp.cpp \
                    src/PolicyStoreImp.hpp \
                    #end

if ENABLE_BETA
    libgeopm_la_SOURCES += $(beta_source_files)
else
    EXTRA_DIST += $(beta_source_files)
endif

geopmagent_SOURCES = src/geopmagent_main.cpp
geopmadmin_SOURCES = src/geopmadmin_main.cpp

pmpi_source_files = src/geopm_ctl.h \
                    src/geopm_pmpi.c \
                    src/geopm_pmpi_helper.cpp \
                    src/geopm_pmpi.h \
                    # end

mpi_source_files = src/MPIComm.cpp \
                   src/MPIComm.hpp \
                   src/MPICommSplit.cpp \
                   src/geopm_mpi_comm_split.h \
                   # end
ompt_source_files = src/OMPT.cpp \
                    src/OMPT.hpp \
                    src/ompt_callback.cpp \
                    # end

geopmctl_SOURCES = src/geopmctl_main.c

if ENABLE_MPI
    libgeopm_la_SOURCES +=$(mpi_source_files)
else
    EXTRA_DIST += $(mpi_source_files)
endif # ENABLE_MPI



if ENABLE_MPI
libgeopm_la_SOURCES += $(pmpi_source_files)
else
EXTRA_DIST += $(pmpi_source_files)
endif # ENABLE_MPI

if ENABLE_OMPT
    libgeopm_la_SOURCES += $(ompt_source_files)
else
    EXTRA_DIST += $(ompt_source_files)
endif

if ENABLE_FORTRAN
    libgeopmfort_la_SOURCES = src/geopm_prof.f90
if ENABLE_MPI
    libgeopmfort_la_SOURCES += src/geopm_pmpi_fortran.c
    libgeopmfort_la_LIBADD = libgeopm.la $(MPI_FLIBS)
endif
    libgeopmfort_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(geopm_abi_version)
    libgeopmfort_la_CFLAGS = $(AM_CFLAGS)
    libgeopmfort_la_FCFLAGS = $(AM_FCFLAGS)
    libgeopmfort_la_CXXFLAGS = $(AM_CXXFLAGS)
else
    EXTRA_DIST += src/geopm_prof.f90 \
                  src/geopm_pmpi_fortran.c \
                  # end
endif

model_source_files = src/All2allModelRegion.cpp \
                     src/All2allModelRegion.hpp \
                     src/BarrierModelRegion.cpp \
                     src/BarrierModelRegion.hpp \
                     src/DGEMMModelRegion.cpp \
                     src/DGEMMModelRegion.hpp \
                     src/GEOPMBenchConfig.cpp \
                     src/GEOPMBenchConfig.hpp \
                     src/IgnoreModelRegion.cpp \
                     src/IgnoreModelRegion.hpp \
                     src/ModelApplication.cpp \
                     src/ModelApplication.hpp \
                     src/ModelRegion.cpp \
                     src/ModelRegion.hpp \
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

geopmbench_SOURCES = $(model_source_files) \
                     src/geopmbench_main.cpp \
                     # end

# CLEAN TARGETS
DISTCLEANFILES = VERSION MANIFEST

CLEAN_LOCAL_TARGETS= clean-local-coverage \
                     clean-local-fortran \
                     clean-local-googletest \
                     # end

clean-local: $(CLEAN_LOCAL_TARGETS)

distclean-local: distclean-local-ruby

clean-local-coverage:
	rm -f `find -name \*.gcno` `find -name \*.gcda`

clean-local-fortran:
	rm -rf geopm_prof.mod

distclean-local-ruby:
	rm -rf ruby

# FORTRAN MODULE TARGET
if ENABLE_MPI
if ENABLE_FORTRAN
BUILT_SOURCES = geopm_prof.mod
geopm_prof.mod: src/geopm_prof.f90
	$(FC) $(AM_FCFLAGS) $(FCFLAGS) -c $<
else
BUILT_SOURCES =
endif
else
BUILT_SOURCES =
endif # ENABLE_MPI

# RPM TARGET
rpm_topdir ?= $(HOME)/rpmbuild
rpm: dist
	mkdir -p $(rpm_topdir)/SOURCES
	mkdir -p $(rpm_topdir)/SPECS
	cp geopm-runtime-$(VERSION).tar.gz $(rpm_topdir)/SOURCES
	cp geopm-runtime.spec $(rpm_topdir)/SPECS
	rpmbuild $(rpmbuild_flags) -ba $(rpm_topdir)/SPECS/geopm-runtime.spec

deb: dist
	rm -rf geopm-runtime-$(VERSION)
	tar xvf geopm-runtime-$(VERSION).tar.gz
	cd geopm-runtime-$(VERSION) && dpkg-buildpackage -us -uc
	rm -rf geopm-runtime-$(VERSION)

geopm-tutorial.tar.gz: $(TUTORIAL_DIST)
	tar --transform='s|^|geopm-|' -zcf $@ $^

if !ENABLE_SANITIZERS
checkprogs: gtest-checkprogs pytest-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)
else
checkprogs: gtest-checkprogs $(check_PROGRAMS) $(check_LTLIBRARIES)
endif

# INCLUDES
check_PROGRAMS =
check_LTLIBRARIES =
noinst_PROGRAMS =
noinst_LTLIBRARIES =
TESTS = test_license.sh

PHONY_TARGETS = clean-local \
                clean-local-coverage \
                distclean-local \
                gh_pages \
                rpm \
                deb \
                # end

include test/Makefile.mk
include docs/Makefile.mk

if !ENABLE_SANITIZERS
include integration/Makefile.mk
include examples/Makefile.mk
include scripts/Makefile.mk
include shell_completion/Makefile.mk
endif

.PHONY: $(PHONY_TARGETS)
