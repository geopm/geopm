#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

check_PROGRAMS += test/geopm_test

if ENABLE_MPI
    check_PROGRAMS += test/geopm_mpi_test_api
endif

#test_env = "LD_LIBRARY_PATH=.libs:openmp/lib:$LD_LIBRARY_PATH"

TESTS += $("LD_LIBRARY_PATH=.libs:openmp/lib:$LD_LIBRARY_PATH" test/geopm_test) \
         $("LD_LIBRARY_PATH=.libs:openmp/lib:$LD_LIBRARY_PATH" test/geopm_mpi_test_api) \
         copying_headers/test-license \
         # if $? != 1 test failed, then gen/parse/equiv of
         # geopm_test.result and geopm_mpi_test_api.result
         # end
#Removed all the bs from Makefile.in ...and
#@todo replace with simplified means
#of generating of this message.
#could be as simple as making and parsing a
#geopm_test.result and geopm_mpi_test_api.result
#if test "$$failed" -ne 0 && test -n "$(PACKAGE_BUGREPORT)"; then \
report="Please report to $(PACKAGE_BUGREPORT)"; \
test `echo "$$report" | wc -c` -le `echo "$$banner" | wc -c` || \
  dashes="$$report"; \
fi; \

EXTRA_DIST += test/InternalProfile.cpp \
              test/InternalProfile.hpp \
              test/MPIInterfaceTest.cpp \
              test/geopm_test.sh \
              test/legacy_whitelist.out \
              test/no_omp_cpu.c \
              test/pmpi_mock.c \
              # end

test_geopm_test_SOURCES = src/EnergyEfficientAgent.cpp \
                          src/EnergyEfficientAgent.hpp \
                          src/EnergyEfficientRegion.cpp \
                          src/EnergyEfficientRegion.hpp \
                          src/ModelParse.cpp \
                          src/ModelParse.hpp \
                          test/AgentFactoryTest.cpp \
                          test/AggTest.cpp \
                          test/ApplicationIOTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/CombinedSignalTest.cpp \
                          test/CommMPIImpTest.cpp \
                          test/ControlMessageTest.cpp \
                          test/ControllerTest.cpp \
                          test/CpuinfoIOGroupTest.cpp \
                          test/EnergyEfficientAgentTest.cpp \
                          test/EnergyEfficientRegionTest.cpp \
                          test/EnvironmentTest.cpp \
                          test/EpochRuntimeRegulatorTest.cpp \
                          test/ExceptionTest.cpp \
                          test/HelperTest.cpp \
                          test/IOGroupTest.cpp \
                          test/MSRIOGroupTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRTest.cpp \
                          test/ManagerIOTest.cpp \
                          test/MockAgent.hpp \
                          test/MockApplicationIO.hpp \
                          test/MockComm.hpp \
                          test/MockControlMessage.hpp \
                          test/MockEpochRuntimeRegulator.hpp \
                          test/MockIOGroup.hpp \
                          test/MockManagerIOSampler.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPlatformTopo.hpp \
                          test/MockPowerBalancer.hpp \
                          test/MockPowerGovernor.hpp \
                          test/MockProfileIOSample.hpp \
                          test/MockProfileSampler.hpp \
                          test/MockProfileTable.hpp \
                          test/MockProfileThreadTable.hpp \
                          test/MockRegionAggregator.hpp \
                          test/MockReporter.hpp \
                          test/MockRuntimeRegulator.hpp \
                          test/MockSampleScheduler.hpp \
                          test/MockSharedMemory.hpp \
                          test/MockSharedMemoryUser.hpp \
                          test/MockTracer.hpp \
                          test/MockTreeComm.hpp \
                          test/MockTreeCommLevel.hpp \
                          test/ModelApplicationTest.cpp \
                          test/MonitorAgentTest.cpp \
                          test/PlatformIOTest.cpp \
                          test/PlatformTopoTest.cpp \
                          test/PowerBalancerAgentTest.cpp \
                          test/PowerBalancerTest.cpp \
                          test/PowerGovernorAgentTest.cpp \
                          test/PowerGovernorTest.cpp \
                          test/ProfileTableTest.cpp \
                          test/ProfileTest.cpp \
                          test/RegionAggregatorTest.cpp \
                          test/ReporterTest.cpp \
                          test/RuntimeRegulatorTest.cpp \
                          test/SampleRegulatorTest.cpp \
                          test/SchedTest.cpp \
                          test/SharedMemoryTest.cpp \
                          test/TimeIOGroupTest.cpp \
                          test/TracerTest.cpp \
                          test/TreeCommLevelTest.cpp \
                          test/TreeCommTest.cpp \
                          test/geopm_test.cpp \
                          test/geopm_test.hpp \
                          # end

test_geopm_test_LDADD = libgeopmpolicy.la \
                        libgmock.a \
                        libgtest.a \
                        # end

test_geopm_test_CPPFLAGS = $(AM_CPPFLAGS) -Iplugin
test_geopm_test_CFLAGS = $(AM_CFLAGS)
test_geopm_test_CXXFLAGS = $(AM_CXXFLAGS)

if GEOPM_DISABLE_INCONSISTENT_OVERRIDE
    test_geopm_test_CFLAGS += -Wno-inconsistent-missing-override
    test_geopm_test_CXXFLAGS += -Wno-inconsistent-missing-override
endif

if ENABLE_MPI
    test_geopm_mpi_test_api_SOURCES = test/MPIInterfaceTest.cpp \
                                      test/geopm_test.cpp \
                                      # end

    test_geopm_mpi_test_api_LDADD = libgeopmpolicy.la \
                                    libgmock.a \
                                    libgtest.a \
                                    # end

    test_geopm_mpi_test_api_LDFLAGS = $(AM_LDFLAGS)
    test_geopm_mpi_test_api_CFLAGS = $(AM_CFLAGS)
    test_geopm_mpi_test_api_CXXFLAGS= $(AM_CXXFLAGS)
endif

# Target for building test programs.
gtest-checkprogs: $(GTEST_TESTS)

PHONY_TARGETS += gtest-checkprogs

$(GTEST_TESTS): test/gtest_links/%:
	mkdir -p test/gtest_links
	ln -s ../geopm_test.sh $@

coverage: check
	lcov --no-external --capture --directory src --output-file coverage.info --rc lcov_branch_coverage=1
	genhtml coverage.info --output-directory coverage --rc lcov_branch_coverage=1

clean-local-gtest-script-links:
	rm -f test/gtest_links/*

CLEAN_LOCAL_TARGETS += clean-local-gtest-script-links

include test/googletest.mk
