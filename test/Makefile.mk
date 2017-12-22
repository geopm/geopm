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

check_PROGRAMS += test/geopm_test

if ENABLE_MPI
    check_PROGRAMS += test/geopm_mpi_test
    check_PROGRAMS += test/geopm_mpi_test_api
endif

GTEST_TESTS = test/gtest_links/PlatformFactoryTest.platform_register \
              test/gtest_links/PlatformFactoryTest.no_supported_platform \
              test/gtest_links/PlatformImpTest.platform_get_name \
              test/gtest_links/PlatformImpTest.platform_get_package \
              test/gtest_links/PlatformImpTest.platform_get_tile \
              test/gtest_links/PlatformImpTest.platform_get_cpu \
              test/gtest_links/PlatformImpTest.platform_get_hyperthreaded \
              test/gtest_links/PlatformImpTest.cpu_msr_read_write \
              test/gtest_links/PlatformImpTest.tile_msr_read_write \
              test/gtest_links/PlatformImpTest.package_msr_read_write \
              test/gtest_links/PlatformImpTest.msr_write_whitelist \
              test/gtest_links/PlatformImpTest.negative_read_no_desc \
              test/gtest_links/PlatformImpTest.negative_write_no_desc \
              test/gtest_links/PlatformImpTest.negative_read_bad_desc \
              test/gtest_links/PlatformImpTest.negative_write_bad_desc \
              test/gtest_links/PlatformImpTest.negative_msr_open \
              test/gtest_links/PlatformImpTest.negative_msr_write_bad_value \
              test/gtest_links/PlatformImpTest.int_type_checks \
              test/gtest_links/PlatformImpTest2.msr_write_restore_read \
              test/gtest_links/PlatformImpTest2.msr_write_backup_file \
              test/gtest_links/PlatformImpTest2.msr_restore_modified_value \
              test/gtest_links/PlatformTopologyTest.cpu_count \
              test/gtest_links/PlatformTopologyTest.negative_num_domain \
              test/gtest_links/CircularBufferTest.buffer_size \
              test/gtest_links/CircularBufferTest.buffer_values \
              test/gtest_links/CircularBufferTest.buffer_capacity \
              test/gtest_links/GlobalPolicyTest.mode_tdp_balance_static \
              test/gtest_links/GlobalPolicyTest.mode_freq_uniform_static \
              test/gtest_links/GlobalPolicyTest.mode_freq_hybrid_static \
              test/gtest_links/GlobalPolicyTest.mode_perf_balance_dynamic \
              test/gtest_links/GlobalPolicyTest.mode_freq_uniform_dynamic \
              test/gtest_links/GlobalPolicyTest.mode_freq_hybrid_dynamic \
              test/gtest_links/GlobalPolicyTest.plugin_strings \
              test/gtest_links/GlobalPolicyTestShmem.mode_tdp_balance_static \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_uniform_static \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_hybrid_static \
              test/gtest_links/GlobalPolicyTestShmem.mode_perf_balance_dynamic \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_uniform_dynamic \
              test/gtest_links/GlobalPolicyTestShmem.mode_freq_hybrid_dynamic \
              test/gtest_links/GlobalPolicyTestShmem.plugin_strings \
              test/gtest_links/GlobalPolicyTest.invalid_policy \
              test/gtest_links/GlobalPolicyTest.c_interface \
              test/gtest_links/GlobalPolicyTest.negative_c_interface \
              test/gtest_links/ExceptionTest.hello \
              test/gtest_links/ProfileTableTest.hello \
              test/gtest_links/ProfileTableTest.name_set_fill_short \
              test/gtest_links/ProfileTableTest.name_set_fill_long \
              test/gtest_links/DeciderFactoryTest.decider_register \
              test/gtest_links/DeciderFactoryTest.no_supported_decider \
              test/gtest_links/RegionTest.identifier \
              test/gtest_links/RegionTest.sample_message \
              test/gtest_links/RegionTest.signal_last \
              test/gtest_links/RegionTest.signal_num \
              test/gtest_links/RegionTest.signal_derivative \
              test/gtest_links/RegionTest.signal_mean \
              test/gtest_links/RegionTest.signal_median \
              test/gtest_links/RegionTest.signal_stddev \
              test/gtest_links/RegionTest.signal_max \
              test/gtest_links/RegionTest.signal_min \
              test/gtest_links/RegionTest.signal_capacity_leaf \
              test/gtest_links/RegionTest.signal_capacity_tree \
              test/gtest_links/RegionTest.signal_invalid_entry \
              test/gtest_links/RegionTest.negative_region_invalid \
              test/gtest_links/RegionTest.negative_signal_invalid \
              test/gtest_links/RegionTest.negative_signal_derivative_tree \
              test/gtest_links/RegionTest.telemetry_timestamp \
              test/gtest_links/SampleRegulatorTest.insert_platform \
              test/gtest_links/SampleRegulatorTest.insert_profile \
              test/gtest_links/SampleRegulatorTest.align_profile \
              test/gtest_links/PolicyTest.num_domain \
              test/gtest_links/PolicyTest.region_id \
              test/gtest_links/PolicyTest.mode \
              test/gtest_links/PolicyTest.frequency \
              test/gtest_links/PolicyTest.tdp_percent \
              test/gtest_links/PolicyTest.affinity \
              test/gtest_links/PolicyTest.goal \
              test/gtest_links/PolicyTest.num_max_perf \
              test/gtest_links/PolicyTest.target \
              test/gtest_links/PolicyTest.target_updated \
              test/gtest_links/PolicyTest.target_valid \
              test/gtest_links/PolicyTest.policy_message \
              test/gtest_links/PolicyTest.converged \
              test/gtest_links/PolicyTest.negative_unsized_vector \
              test/gtest_links/PolicyTest.negative_index_oob \
              test/gtest_links/BalancingDeciderTest.name \
              test/gtest_links/BalancingDeciderTest.clone \
              test/gtest_links/BalancingDeciderTest.supported \
              test/gtest_links/BalancingDeciderTest.new_policy_message \
              test/gtest_links/BalancingDeciderTest.update_policy \
              test/gtest_links/GoverningDeciderTest.decider_is_supported \
              test/gtest_links/GoverningDeciderTest.name \
              test/gtest_links/GoverningDeciderTest.clone \
              test/gtest_links/GoverningDeciderTest.1_socket_under_budget \
              test/gtest_links/GoverningDeciderTest.1_socket_over_budget \
              test/gtest_links/GoverningDeciderTest.2_socket_under_budget \
              test/gtest_links/GoverningDeciderTest.2_socket_over_budget \
              test/gtest_links/SimpleFreqDeciderTest.map \
              test/gtest_links/SimpleFreqDeciderTest.decider_is_supported \
              test/gtest_links/SimpleFreqDeciderTest.name \
              test/gtest_links/SimpleFreqDeciderTest.clone \
              test/gtest_links/SimpleFreqDeciderTest.hint \
              test/gtest_links/AdaptiveFreqDeciderTest.adaptive_branch \
              test/gtest_links/SharedMemoryTest.invalid_construction \
              test/gtest_links/SharedMemoryTest.share_data \
              test/gtest_links/SharedMemoryTest.share_data_ipc \
              test/gtest_links/EnvironmentTest.construction0 \
              test/gtest_links/EnvironmentTest.construction1 \
              test/gtest_links/SchedTest.test_proc_cpuset_0 \
              test/gtest_links/SchedTest.test_proc_cpuset_1 \
              test/gtest_links/SchedTest.test_proc_cpuset_2 \
              test/gtest_links/SchedTest.test_proc_cpuset_3 \
              test/gtest_links/SchedTest.test_proc_cpuset_4 \
              test/gtest_links/SchedTest.test_proc_cpuset_5 \
              test/gtest_links/SchedTest.test_proc_cpuset_6 \
              test/gtest_links/SchedTest.test_proc_cpuset_7 \
              test/gtest_links/SchedTest.test_proc_cpuset_8 \
              test/gtest_links/ControlMessageTest.step \
              test/gtest_links/ControlMessageTest.wait \
              test/gtest_links/ControlMessageTest.cpu_rank \
              test/gtest_links/ControlMessageTest.is_sample_begin \
              test/gtest_links/ControlMessageTest.is_sample_end \
              test/gtest_links/ControlMessageTest.is_name_begin \
              test/gtest_links/ControlMessageTest.is_shutdown \
              test/gtest_links/ControlMessageTest.loop_begin_0 \
              test/gtest_links/ControlMessageTest.loop_begin_1 \
              test/gtest_links/CommMPIImpTest.mpi_comm_ops \
              test/gtest_links/CommMPIImpTest.mpi_reduce \
              test/gtest_links/CommMPIImpTest.mpi_allreduce \
              test/gtest_links/CommMPIImpTest.mpi_gather \
              test/gtest_links/CommMPIImpTest.mpi_gatherv \
              test/gtest_links/CommMPIImpTest.mpi_broadcast \
              test/gtest_links/CommMPIImpTest.mpi_cart_ops \
              test/gtest_links/CommMPIImpTest.mpi_dims_create \
              test/gtest_links/CommMPIImpTest.mpi_mem_ops \
              test/gtest_links/CommMPIImpTest.mpi_barrier \
              test/gtest_links/CommMPIImpTest.mpi_win_ops \
              test/gtest_links/PlatformIOTest.freq_ctl \
              test/gtest_links/PlatformIOTest.whitelist \
              test/gtest_links/PlatformIOTest.time_signal \
              test/gtest_links/MSRIOTest.read_aligned \
              test/gtest_links/MSRIOTest.read_unaligned \
              test/gtest_links/MSRIOTest.write \
              test/gtest_links/MSRIOTest.read_batch \
              test/gtest_links/MSRIOTest.write_batch \
              test/gtest_links/MSRTest.msr \
              test/gtest_links/MSRTest.msr_signal \
              test/gtest_links/MSRTest.msr_control \
              test/gtest_links/AdaptiveFreqRegionTest.freq_starts_at_maximum \
              test/gtest_links/AdaptiveFreqRegionTest.update_ignores_nan_sample \
              test/gtest_links/AdaptiveFreqRegionTest.only_changes_freq_after_enough_samples \
              test/gtest_links/AdaptiveFreqRegionTest.freq_does_not_go_below_min \
              test/gtest_links/AdaptiveFreqRegionTest.performance_decreases_freq_steps_back_up \
              test/gtest_links/AdaptiveFreqRegionTest.energy_increases_freq_steps_back_up \
              test/gtest_links/AdaptiveFreqRegionTest.after_too_many_increase_freq_stays_at_higher \
              test/gtest_links/RuntimeRegulatorTest.exceptions \
              test/gtest_links/RuntimeRegulatorTest.all_in_and_out \
              test/gtest_links/RuntimeRegulatorTest.all_reenter \
              test/gtest_links/RuntimeRegulatorTest.one_rank_reenter_and_exit \
              test/gtest_links/RuntimeRegulatorTest.config_rank_then_workers \
              # end

if ENABLE_MPI
GTEST_TESTS += test/gtest_links/MPITreeCommunicatorTest.hello \
               test/gtest_links/MPITreeCommunicatorTest.send_policy_down \
               test/gtest_links/MPITreeCommunicatorTest.send_sample_up \
               test/gtest_links/MPIInterfaceTest.geopm_api \
               test/gtest_links/MPIInterfaceTest.mpi_api \
               # end
endif

TESTS += $(GTEST_TESTS) \
         copying_headers/test-license \
         # end

EXTRA_DIST += test/geopm_test.sh \
              test/MPITreeCommunicatorTest.cpp \
              test/MPIInterfaceTest.cpp \
              test/no_omp_cpu.c \
              test/default_policy.json \
              test/invalid_policy.json \
              test/legacy_whitelist.out \
              # end

test_geopm_test_SOURCES = test/geopm_test.cpp \
                          test/PlatformFactoryTest.cpp \
                          test/PlatformImpTest.cpp \
                          test/PlatformTest.cpp \
                          test/PlatformTopologyTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/GlobalPolicyTest.cpp \
                          test/ExceptionTest.cpp \
                          test/ProfileTableTest.cpp \
                          test/DeciderFactoryTest.cpp \
                          test/SampleRegulatorTest.cpp \
                          test/RegionTest.cpp \
                          test/PolicyTest.cpp \
                          plugin/BalancingDecider.hpp \
                          test/BalancingDeciderTest.cpp \
                          plugin/GoverningDecider.hpp \
                          test/GoverningDeciderTest.cpp \
                          plugin/SimpleFreqDecider.hpp \
                          test/MockRegion.hpp \
                          test/MockPolicy.hpp \
                          test/SimpleFreqDeciderTest.cpp \
                          test/MockComm.hpp \
                          test/MockMSRIO.hpp \
                          test/MockPlatform.hpp \
                          test/MockProfileSampler.hpp \
                          test/MockGlobalPolicy.hpp \
                          test/MockPlatformImp.hpp \
                          test/MockPlatformTopology.hpp \
                          test/SharedMemoryTest.cpp \
                          test/EnvironmentTest.cpp \
                          test/SchedTest.cpp \
                          test/ControlMessageTest.cpp \
                          test/CommMPIImpTest.cpp \
                          test/PlatformIOTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRTest.cpp \
                          plugin/AdaptiveFreqRegion.hpp \
                          plugin/AdaptiveFreqRegion.cpp \
                          test/AdaptiveFreqRegionTest.cpp \
                          test/RuntimeRegulatorTest.cpp \
                          # end

test_geopm_test_LDADD = libgtest.a \
                        libgmock.a \
                        libgeopmpolicy.la \
                        # end

test_geopm_test_CPPFLAGS = $(AM_CPPFLAGS) -Iplugin
test_geopm_test_CFLAGS = $(AM_CFLAGS)
test_geopm_test_CXXFLAGS = $(AM_CXXFLAGS)

if GEOPM_DISABLE_NULL_PTR
    test_geopm_test_CFLAGS += -fno-delete-null-pointer-checks
    test_geopm_test_CXXFLAGS += -fno-delete-null-pointer-checks
endif
if ENABLE_OPENMP
    test_geopm_static_modes_test_SOURCES = test/geopm_static_modes_test.cpp
    test_geopm_static_modes_test_LDADD = libgeopmpolicy.la
    check_PROGRAMS += test/geopm_static_modes_test
endif


if ENABLE_MPI
    test_geopm_mpi_test_SOURCES = test/geopm_mpi_test.cpp \
                                  test/MPITreeCommunicatorTest.cpp \
                                  # end

    test_geopm_mpi_test_api_SOURCES = test/geopm_test.cpp \
                                      test/MPIInterfaceTest.cpp \
                                      # end

    test_geopm_mpi_test_LDADD = libgtest.a \
                                libgmock.a \
                                libgeopm.la \
                                $(MPI_CXXLIBS) \
                                # end

    test_geopm_mpi_test_api_LDADD = libgtest.a \
                                    libgmock.a \
                                    libgeopm.la \
                                    # end

    test_geopm_mpi_test_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS)
    test_geopm_mpi_test_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    test_geopm_mpi_test_CXXFLAGS= $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
    test_geopm_mpi_test_api_LDFLAGS = $(AM_LDFLAGS)
    test_geopm_mpi_test_api_CFLAGS = $(AM_CFLAGS)
    test_geopm_mpi_test_api_CXXFLAGS= $(AM_CXXFLAGS)
if GEOPM_DISABLE_NULL_PTR
    test_geopm_mpi_test_CFLAGS += -fno-delete-null-pointer-checks
    test_geopm_mpi_test_CXXFLAGS += -fno-delete-null-pointer-checks
    test_geopm_mpi_test_api_CFLAGS += -fno-delete-null-pointer-checks
    test_geopm_mpi_test_api_CXXFLAGS += -fno-delete-null-pointer-checks
endif
endif

# Target for building test programs.
gtest-checkprogs: $(GTEST_TESTS)

PHONY_TARGETS += gtest-checkprogs

$(GTEST_TESTS): test/gtest_links/%:
	mkdir -p test/gtest_links
	ln -s ../geopm_test.sh $@

coverage: check
	lcov --no-external --capture --directory src --directory plugin --output-file coverage.info --rc lcov_branch_coverage=1
	genhtml coverage.info --output-directory coverage --rc lcov_branch_coverage=1

clean-local-gtest-script-links:
	rm -f test/gtest_links/*

CLEAN_LOCAL_TARGETS += clean-local-gtest-script-links

include test/googletest.mk
include test/plugin/Makefile.mk
