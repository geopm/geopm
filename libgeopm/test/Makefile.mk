#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

check_PROGRAMS += test/geopm_test

if ENABLE_MPI
    check_PROGRAMS += test/geopm_mpi_test_api
endif

TESTS_ENVIRONMENT = PYTHON='$(PYTHON)'

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
				  $(top_srcdir)/build-aux/tap-driver.sh
TESTS += test/geopm_test.test \
         # end

EXTRA_DIST += test/InternalProfile.cpp \
              test/InternalProfile.hpp \
              test/MPIInterfaceTest.cpp \
              test/no_omp_cpu.c \
              test/pmpi_mock.c \
              test/ProxyEpochRecordFilterTest.tutorial_2_profile_trace \
              test/EditDistPeriodicityDetectorTest.0_pattern_a.trace \
              test/EditDistPeriodicityDetectorTest.1_pattern_ab.trace \
              test/EditDistPeriodicityDetectorTest.2_pattern_abb.trace \
              test/EditDistPeriodicityDetectorTest.3_pattern_abcdc.trace \
              test/EditDistPeriodicityDetectorTest.4_pattern_ababc.trace \
              test/EditDistPeriodicityDetectorTest.5_pattern_abababc.trace \
              test/EditDistPeriodicityDetectorTest.6_pattern_add1.trace \
              test/EditDistPeriodicityDetectorTest.7_pattern_add2.trace \
              test/EditDistPeriodicityDetectorTest.8_pattern_subtract1.trace \
              test/EditDistPeriodicityDetectorTest.fft_small.trace \
              test/EditDistPeriodicityDetectorTest.cpp \
              test/geopm_test.test \
              # end

test_geopm_test_SOURCES = test/AccumulatorTest.cpp \
                          test/AdminTest.cpp \
                          test/AgentFactoryTest.cpp \
                          test/ApplicationIOTest.cpp \
                          test/ApplicationRecordLogTest.cpp \
                          test/ApplicationSamplerTest.cpp \
                          test/ApplicationStatusTest.cpp \
                          test/CommMPIImpTest.cpp \
                          test/CommNullImpTest.cpp \
                          test/ControllerTest.cpp \
                          test/CSVTest.cpp \
                          test/DebugIOGroupTest.cpp \
                          test/DenseLayerTest.cpp \
                          test/DomainNetMapTest.cpp \
                          test/EditDistEpochRecordFilterTest.cpp \
                          test/EditDistPeriodicityDetectorTest.cpp \
                          test/EndpointTest.cpp \
                          test/EndpointPolicyTracerTest.cpp \
                          test/EndpointUserTest.cpp \
                          test/EnvironmentTest.cpp \
                          test/EpochIOGroupTest.cpp \
                          test/EpochIOGroupIntegrationTest.cpp \
                          test/FilePolicyTest.cpp \
                          test/FrequencyGovernorTest.cpp \
                          test/FrequencyMapAgentTest.cpp \
                          test/FrequencyTimeBalancerTest.cpp \
                          test/InitControlTest.cpp \
                          test/LocalNeuralNetTest.cpp \
                          test/MockAgent.hpp \
                          test/MockApplicationIO.hpp \
                          test/MockApplicationRecordLog.hpp \
                          test/MockApplicationSampler.cpp \
                          test/MockApplicationSampler.hpp \
                          test/MockApplicationStatus.hpp \
                          test/MockComm.hpp \
                          test/MockDenseLayer.hpp \
                          test/MockDomainNetMap.hpp \
                          test/MockEndpoint.hpp \
                          test/MockEndpointPolicyTracer.hpp \
                          test/MockEndpointUser.hpp \
                          test/MockFrequencyLimitDetector.hpp \
                          test/MockFrequencyGovernor.hpp \
                          test/MockFrequencyTimeBalancer.hpp \
                          test/MockLocalNeuralNet.hpp \
                          test/MockIOGroup.hpp \
                          test/MockInitControl.hpp \
                          test/MockNNFactory.hpp \
                          test/MockPlatformTopo.cpp \
                          test/MockPlatformTopo.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPowerBalancer.hpp \
                          test/MockPowerGovernor.hpp \
                          test/MockProcessRegionAggregator.hpp \
                          test/MockProfileTracer.hpp \
                          test/MockRecordFilter.hpp \
                          test/MockRegionHintRecommender.hpp \
                          test/MockReporter.hpp \
                          test/MockSampleAggregator.hpp \
                          test/MockScheduler.hpp \
                          test/MockServiceProxy.hpp \
                          test/MockSharedMemory.hpp \
                          test/MockSSTClosGovernor.hpp \
                          test/MockTensorMath.hpp \
                          test/MockTracer.hpp \
                          test/MockTracer.hpp \
                          test/MockTreeComm.hpp \
                          test/MockTreeCommLevel.hpp \
                          test/MockWaiter.hpp \
                          test/ModelApplicationTest.cpp \
                          test/MonitorAgentTest.cpp \
                          test/OptionParserTest.cpp \
                          test/PowerBalancerAgentTest.cpp \
                          test/PowerBalancerTest.cpp \
                          test/PowerGovernorAgentTest.cpp \
                          test/PowerGovernorTest.cpp \
                          test/ProfileIOGroupTest.cpp \
                          test/ProfileTest.cpp \
                          test/ProfileTracerTest.cpp \
                          test/ProxyEpochRecordFilterTest.cpp \
                          test/ProcessRegionAggregatorTest.cpp \
                          test/RecordFilterTest.cpp \
                          test/RegionHintRecommenderTest.cpp \
                          test/ReporterTest.cpp \
                          test/SampleAggregatorTest.cpp \
                          test/SchedTest.cpp \
                          test/SSTClosGovernorTest.cpp \
                          test/SSTFrequencyLimitDetectorTest.cpp \
                          test/TensorMathTest.cpp \
                          test/TensorOneDTest.cpp \
                          test/TensorOneDIntegrationTest.cpp \
                          test/TensorOneDMatcher.cpp \
                          test/TensorOneDMatcher.hpp \
                          test/TensorTwoDTest.cpp \
                          test/TensorTwoDIntegrationTest.cpp \
                          test/TensorTwoDMatcher.cpp \
                          test/TensorTwoDMatcher.hpp \
                          test/TracerTest.cpp \
                          test/TreeCommLevelTest.cpp \
                          test/TreeCommTest.cpp \
                          test/TRLFrequencyLimitDetectorTest.cpp \
                          test/ValidateRecordTest.cpp \
                          test/WaiterTest.cpp \
                          test/geopm_test.cpp \
                          test/geopm_test_helper.cpp \
                          test/geopm_test.hpp \
                          # end

beta_test_sources = test/CPUActivityAgentTest.cpp \
                    test/DaemonTest.cpp \
                    test/FFNetAgentTest.cpp \
                    test/GPUActivityAgentTest.cpp \
                    test/MockPolicyStore.hpp \
                    test/PolicyStoreImpTest.cpp \
                    # end

if ENABLE_BETA
    test_geopm_test_SOURCES += $(beta_test_sources)
else
    EXTRA_DIST += $(beta_test_sources)
endif

if ENABLE_OMPT
    test_geopm_test_SOURCES += test/ELFTest.cpp
else
    EXTRA_DIST += test/ELFTest.cpp
endif

test_geopm_test_SOURCES += src/Profile.cpp \
                           include/geopm/Profile.hpp \
                           # endif

test_geopm_test_LDADD = libgeopm.la \
                        libgmock.a \
                        libgtest.a \
                        # end

test_geopm_test_CPPFLAGS = $(AM_CPPFLAGS) -Iplugin
test_geopm_test_CFLAGS = $(AM_CFLAGS)
test_geopm_test_CXXFLAGS = $(AM_CXXFLAGS)

if ENABLE_MPI
    test_geopm_mpi_test_api_SOURCES = test/MPIInterfaceTest.cpp \
                                      test/geopm_test.cpp \
                                      # end

    test_geopm_mpi_test_api_LDADD = libgeopm.la \
                                    libgmock.a \
                                    libgtest.a \
                                    # end

    test_geopm_mpi_test_api_LDFLAGS = $(AM_LDFLAGS)
    test_geopm_mpi_test_api_CFLAGS = $(AM_CFLAGS)
    test_geopm_mpi_test_api_CXXFLAGS= $(AM_CXXFLAGS)
else
    EXTRA_DIST += test/MPIInterfaceTest.cpp \
                  test/geopm_test.cpp \
                  # end
endif

init-coverage: all
	lcov --no-external --capture --initial --directory src --output-file coverage-base-initial.info

if ENABLE_COVERAGE
check: init-coverage
endif

coverage: init-coverage check
	lcov --no-external --capture --directory src --output-file coverage-base.info
	lcov -a coverage-base-initial.info -a coverage-base.info --output-file coverage-base-combined.info
	lcov --remove coverage-base-combined.info "$$(realpath $$(pwd))/src/geopm_pmpi_fortran.c" --output-file coverage-base-combined-filtered.info
	genhtml coverage-base-combined-filtered.info --output-directory coverage-base --legend -t $(VERSION) -f

include test/googletest.mk
