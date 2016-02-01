#  Copyright (c) 2015, 2016, Intel Corporation
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
    check_PROGRAMS += test/geopm_mpi_test
endif

GTEST_TESTS = test/gtest_links/ObservationTest.hello_mean \
              test/gtest_links/ObservationTest.hello_median \
              test/gtest_links/ObservationTest.hello_stddev \
              test/gtest_links/ObservationTest.hello_max \
              test/gtest_links/ObservationTest.hello_min \
              test/gtest_links/ObservationTest.negative_not_allocated \
              test/gtest_links/ObservationTest.negative_empty \
              test/gtest_links/PlatformFactoryTest.platform_register \
              test/gtest_links/PlatformFactoryTest.no_supported_platform \
              test/gtest_links/PlatformTest.transform_init \
              test/gtest_links/PlatformImpTest.platform_get_name \
              test/gtest_links/PlatformImpTest.platform_get_sockets \
              test/gtest_links/PlatformImpTest.platform_get_tiles \
              test/gtest_links/PlatformImpTest.platform_get_cpus \
              test/gtest_links/PlatformImpTest.platform_get_hyperthreaded \
              test/gtest_links/PlatformImpTest.platform_get_offsets \
              test/gtest_links/PlatformImpTest.cpu_msr_read_write \
              test/gtest_links/PlatformImpTest.tile_msr_read_write \
              test/gtest_links/PlatformImpTest.socket_msr_read_write \
              test/gtest_links/PlatformImpTest.write_msr_whitelist \
              test/gtest_links/PlatformImpTest.negative_read_no_desc \
              test/gtest_links/PlatformImpTest.negative_write_no_desc \
              test/gtest_links/PlatformImpTest.negative_read_bad_desc \
              test/gtest_links/PlatformImpTest.negative_write_bad_desc \
              test/gtest_links/PlatformImpTest.negative_open_msr \
              test/gtest_links/PlatformTopologyTest.cpu_count \
              test/gtest_links/PlatformTopologyTest.get_cpus \
              test/gtest_links/PlatformTopologyTest.negative_num_domain \
              test/gtest_links/PlatformTopologyTest.negative_domain_by_type \
              test/gtest_links/CircularBufferTest.buffer_size \
              test/gtest_links/CircularBufferTest.buffer_values \
              test/gtest_links/CircularBufferTest.buffer_capacity \
              test/gtest_links/GlobalPolicyTest.mode_tdp_balance_static \
              test/gtest_links/GlobalPolicyTest.mode_freq_uniform_static \
              test/gtest_links/GlobalPolicyTest.mode_freq_hybrid_static \
              test/gtest_links/GlobalPolicyTest.mode_perf_balance_dynamic \
              test/gtest_links/GlobalPolicyTest.mode_freq_uniform_dynamic \
              test/gtest_links/GlobalPolicyTest.mode_freq_hybrid_dynamic \
              test/gtest_links/GlobalPolicyTestShmem.mode_tdp_balance_static \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_uniform_static \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_hybrid_static \
              test/gtest_links/GlobalPolicyTestShmem.mode_perf_balance_dynamic \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_uniform_dynamic \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_hybrid_dynamic \
              test/gtest_links/GlobalPolicyTest.c_interface \
              test/gtest_links/GlobalPolicyTest.negative_c_interface \
              test/gtest_links/ExceptionTest.hello \
              test/gtest_links/MPITreeCommunicatorTest.hello \
              test/gtest_links/MPITreeCommunicatorTest.send_policy_down \
              test/gtest_links/MPITreeCommunicatorTest.send_sample_up \
              test/gtest_links/MPITreeCommunicatorTestShmem.hello \
              test/gtest_links/LockingHashTableTest.hello \
              test/gtest_links/LockingHashTableTest.name_set_fill_short \
              test/gtest_links/LockingHashTableTest.name_set_fill_long \
              test/gtest_links/DeciderFactoryTest.decider_register \
              test/gtest_links/DeciderFactoryTest.no_supported_decider \
              test/gtest_links/MPISharedMemoryTest.hello \
              test/gtest_links/MPIProfileTest.noctl \
              test/gtest_links/SampleRegulatorTest.insert_platform \
              test/gtest_links/SampleRegulatorTest.insert_profile_unsync \
              test/gtest_links/SampleRegulatorTest.insert_profile \
              test/gtest_links/SampleRegulatorTest.align_profile \
              test/gtest_links/SampleRegulatorTest.transform \
              # end

TESTS += $(GTEST_TESTS) \
         copying_headers/test-license \
         # end

EXTRA_DIST += test/geopm_test.sh \
              test/geopm_prof_example_openmp_static.c \
              test/MPITreeCommunicatorTest.cpp \
              test/MPIControllerTest.cpp \
              test/no_omp_cpu.c \
              # end

test_geopm_test_SOURCES = test/geopm_test.cpp \
                          test/ObservationTest.cpp \
                          test/PlatformFactoryTest.cpp \
                          test/PlatformImpTest.cpp \
                          test/PlatformTest.cpp \
                          test/PlatformTopologyTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/GlobalPolicyTest.cpp \
                          test/ExceptionTest.cpp \
                          test/LockingHashTableTest.cpp \
                          src/LockingHashTable.hpp \
                          test/DeciderFactoryTest.cpp \
                          test/SampleRegulatorTest.cpp \
                          test/MockPlatform.hpp \
                          test/MockPlatformImp.hpp \
                          test/MockPlatformTopology.hpp \
                          # end

test_geopm_test_LDADD = libgtest.a \
                        libgmock.a \
                        libgeopmpolicy.la \
                        # end

if ENABLE_OPENMP
    test_geopm_static_modes_test_SOURCES = test/geopm_static_modes_test.cpp
    test_geopm_static_modes_test_LDADD = libgeopmpolicy.la
    check_PROGRAMS += test/geopm_static_modes_test
endif


if ENABLE_MPI
    test_geopm_mpi_test_SOURCES = test/geopm_mpi_test.cpp \
                                  test/MPITreeCommunicatorTest.cpp \
                                  test/MPISharedMemoryTest.cpp \
                                  test/MPIProfileTest.cpp \
                                  # end

    test_geopm_mpi_test_LDADD = libgtest.a \
                                libgmock.a \
                                libgeopm.la \
                                # end

    test_geopm_mpi_test_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CFLAGS)
    test_geopm_mpi_test_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    test_geopm_mpi_test_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    test_geopm_mpi_test_CXXFLAGS= $(AM_CXXFLAGS) $(MPI_CXXFLAGS)

endif

# Target for building test programs.
checkprogs: $(check_PROGRAMS) $(GTEST_TESTS)
.PHONY: checkprogs

$(GTEST_TESTS): test/gtest_links/%:
	mkdir -p test/gtest_links
	ln -s ../geopm_test.sh $@

coverage: check
	lcov --no-external --capture --directory src --output-file coverage.info --rc lcov_branch_coverage=1
	genhtml coverage.info --output-directory coverage --rc lcov_branch_coverage=1

clean-local: clean-local-script-links

clean-local-script-links:
	rm -f test/gtest_links/*

include test/googletest.mk
