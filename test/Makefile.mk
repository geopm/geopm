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

GTEST_TESTS = test/gtest_links/AgentFactoryTest.static_info_monitor \
              test/gtest_links/AgentFactoryTest.static_info_balancing \
              test/gtest_links/AggTest.agg_function \
              test/gtest_links/ApplicationIOTest.passthrough \
              test/gtest_links/CircularBufferTest.buffer_capacity \
              test/gtest_links/CircularBufferTest.buffer_size \
              test/gtest_links/CircularBufferTest.buffer_values \
              test/gtest_links/CombinedSignalTest.sample_flat_derivative \
              test/gtest_links/CombinedSignalTest.sample_slope_derivative \
              test/gtest_links/CombinedSignalTest.sample_sum \
              test/gtest_links/CommMPIImpTest.mpi_allreduce \
              test/gtest_links/CommMPIImpTest.mpi_barrier \
              test/gtest_links/CommMPIImpTest.mpi_broadcast \
              test/gtest_links/CommMPIImpTest.mpi_cart_ops \
              test/gtest_links/CommMPIImpTest.mpi_comm_ops \
              test/gtest_links/CommMPIImpTest.mpi_dims_create \
              test/gtest_links/CommMPIImpTest.mpi_gather \
              test/gtest_links/CommMPIImpTest.mpi_gatherv \
              test/gtest_links/CommMPIImpTest.mpi_mem_ops \
              test/gtest_links/CommMPIImpTest.mpi_reduce \
              test/gtest_links/CommMPIImpTest.mpi_win_ops \
              test/gtest_links/CNLIOGroupTest.valid_signals \
              test/gtest_links/CNLIOGroupTest.read_signal \
              test/gtest_links/CNLIOGroupTest.push_signal \
              test/gtest_links/CNLIOGroupTest.parse_energy \
              test/gtest_links/CNLIOGroupTest.parse_power \
              test/gtest_links/ControlMessageTest.cpu_rank \
              test/gtest_links/ControlMessageTest.is_name_begin \
              test/gtest_links/ControlMessageTest.is_sample_begin \
              test/gtest_links/ControlMessageTest.is_sample_end \
              test/gtest_links/ControlMessageTest.is_shutdown \
              test/gtest_links/ControlMessageTest.loop_begin_0 \
              test/gtest_links/ControlMessageTest.loop_begin_1 \
              test/gtest_links/ControlMessageTest.step \
              test/gtest_links/ControlMessageTest.wait \
              test/gtest_links/ControllerTest.single_node \
              test/gtest_links/ControllerTest.two_level_controller_0 \
              test/gtest_links/ControllerTest.two_level_controller_1 \
              test/gtest_links/ControllerTest.two_level_controller_2 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_freq \
              test/gtest_links/CpuinfoIOGroupTest.parse_error_no_sticker \
              test/gtest_links/CpuinfoIOGroupTest.parse_error_sticker_bad_path \
              test/gtest_links/CpuinfoIOGroupTest.parse_sticker_missing_newline \
              test/gtest_links/CpuinfoIOGroupTest.parse_sticker_multiple_ghz \
              test/gtest_links/CpuinfoIOGroupTest.parse_sticker_multiple_model_name \
              test/gtest_links/CpuinfoIOGroupTest.parse_sticker_with_at \
              test/gtest_links/CpuinfoIOGroupTest.parse_sticker_with_ghz_space \
              test/gtest_links/CpuinfoIOGroupTest.parse_sticker_without_at \
              test/gtest_links/CpuinfoIOGroupTest.plugin \
              test/gtest_links/CpuinfoIOGroupTest.valid_signals \
              test/gtest_links/CSVTest.buffer \
              test/gtest_links/CSVTest.columns \
              test/gtest_links/CSVTest.header \
              test/gtest_links/CSVTest.negative \
              test/gtest_links/DebugIOGroupTest.is_valid \
              test/gtest_links/DebugIOGroupTest.push \
              test/gtest_links/DebugIOGroupTest.read_signal \
              test/gtest_links/DebugIOGroupTest.register_signal_error \
              test/gtest_links/DebugIOGroupTest.sample \
              test/gtest_links/ELFTest.symbols_exist \
              test/gtest_links/ELFTest.symbol_lookup \
              test/gtest_links/EnergyEfficientAgentTest.aggregate_sample \
              test/gtest_links/EnergyEfficientAgentTest.do_write_batch \
              test/gtest_links/EnergyEfficientAgentTest.enforce_policy \
              test/gtest_links/EnergyEfficientAgentTest.split_policy_changed \
              test/gtest_links/EnergyEfficientAgentTest.split_policy_errors \
              test/gtest_links/EnergyEfficientAgentTest.split_policy_unchanged \
              test/gtest_links/EnergyEfficientAgentTest.static_methods \
              test/gtest_links/EnergyEfficientAgentTest.validate_policy_clamp \
              test/gtest_links/EnergyEfficientAgentTest.validate_policy_default \
              test/gtest_links/EnergyEfficientRegionTest.freq_starts_at_maximum \
              test/gtest_links/EnergyEfficientRegionTest.invalid_perf_margin \
              test/gtest_links/EnergyEfficientRegionTest.update_ignores_nan_sample \
              test/gtest_links/EnvironmentTest.internal_defaults \
              test/gtest_links/EnvironmentTest.user_only \
              test/gtest_links/EnvironmentTest.user_only_do_profile \
              test/gtest_links/EnvironmentTest.user_only_do_profile_name \
              test/gtest_links/EnvironmentTest.default_only \
              test/gtest_links/EnvironmentTest.override_only \
              test/gtest_links/EnvironmentTest.default_and_override \
              test/gtest_links/EnvironmentTest.user_default_and_override \
              test/gtest_links/EnvironmentTest.invalid_ctl \
              test/gtest_links/EpochRuntimeRegulatorTest.all_ranks_enter_exit \
              test/gtest_links/EpochRuntimeRegulatorTest.epoch_runtime \
              test/gtest_links/EpochRuntimeRegulatorTest.invalid_ranks \
              test/gtest_links/EpochRuntimeRegulatorTest.rank_enter_exit_trace \
              test/gtest_links/EpochRuntimeRegulatorTest.unknown_region \
              test/gtest_links/ExceptionTest.hello \
              test/gtest_links/FilePolicyTest.parse_json_file \
              test/gtest_links/FilePolicyTest.negative_bad_files \
              test/gtest_links/FilePolicyTest.negative_parse_json_file \
              test/gtest_links/FrequencyGovernorTest.frequency_control_domain_default \
              test/gtest_links/FrequencyGovernorTest.adjust_platform \
              test/gtest_links/FrequencyGovernorTest.adjust_platform_clamping \
              test/gtest_links/FrequencyGovernorTest.adjust_platform_error \
              test/gtest_links/FrequencyGovernorTest.frequency_bounds_in_range \
              test/gtest_links/FrequencyGovernorTest.frequency_bounds_invalid \
              test/gtest_links/FrequencyGovernorTest.validate_policy \
              test/gtest_links/FrequencyMapAgentTest.hint \
              test/gtest_links/FrequencyMapAgentTest.map \
              test/gtest_links/FrequencyMapAgentTest.name \
              test/gtest_links/FrequencyMapAgentTest.enforce_policy \
              test/gtest_links/HelperTest.string_begins_with \
              test/gtest_links/HelperTest.string_ends_with \
              test/gtest_links/HelperTest.string_split \
              test/gtest_links/IOGroupTest.control_names_are_valid \
              test/gtest_links/IOGroupTest.controls_have_descriptions \
              test/gtest_links/IOGroupTest.signal_names_are_valid \
              test/gtest_links/IOGroupTest.signals_have_agg_functions \
              test/gtest_links/IOGroupTest.signals_have_descriptions \
              test/gtest_links/IOGroupTest.signals_have_format_functions \
              test/gtest_links/MSRIOGroupTest.adjust \
              test/gtest_links/MSRIOGroupTest.control_alias \
              test/gtest_links/MSRIOGroupTest.control_error \
              test/gtest_links/MSRIOGroupTest.cpuid \
              test/gtest_links/MSRIOGroupTest.format_function \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs_error_fields \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs_error_msrs \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs_error_top_level \
              test/gtest_links/MSRIOGroupTest.push_control \
              test/gtest_links/MSRIOGroupTest.push_signal \
              test/gtest_links/MSRIOGroupTest.read_signal \
              test/gtest_links/MSRIOGroupTest.register_msr_control \
              test/gtest_links/MSRIOGroupTest.register_msr_signal \
              test/gtest_links/MSRIOGroupTest.sample \
              test/gtest_links/MSRIOGroupTest.sample_raw \
              test/gtest_links/MSRIOGroupTest.signal_alias \
              test/gtest_links/MSRIOGroupTest.signal_error \
              test/gtest_links/MSRIOGroupTest.supported_cpuid \
              test/gtest_links/MSRIOGroupTest.whitelist \
              test/gtest_links/MSRIOGroupTest.write_control \
              test/gtest_links/MSRIOTest.read_aligned \
              test/gtest_links/MSRIOTest.read_batch \
              test/gtest_links/MSRIOTest.read_unaligned \
              test/gtest_links/MSRIOTest.write \
              test/gtest_links/MSRIOTest.write_batch \
              test/gtest_links/MSRTest.msr \
              test/gtest_links/MSRTest.msr_control \
              test/gtest_links/MSRTest.msr_overflow \
              test/gtest_links/MSRTest.msr_signal \
              test/gtest_links/MSRTest.string_to_function \
              test/gtest_links/MSRTest.string_to_units \
              test/gtest_links/PolicyStoreImpTest.self_consistent \
              test/gtest_links/PolicyStoreImpTest.update_policy \
              test/gtest_links/PolicyStoreImpTest.table_precedence \
              test/gtest_links/ModelApplicationTest.parse_config_errors \
              test/gtest_links/MonitorAgentTest.policy_names \
              test/gtest_links/MonitorAgentTest.sample_names \
              test/gtest_links/PlatformIOTest.adjust \
              test/gtest_links/PlatformIOTest.adjust_agg \
              test/gtest_links/PlatformIOTest.domain_type \
              test/gtest_links/PlatformIOTest.push_control \
              test/gtest_links/PlatformIOTest.push_control_agg \
              test/gtest_links/PlatformIOTest.push_signal \
              test/gtest_links/PlatformIOTest.push_signal_agg \
              test/gtest_links/PlatformIOTest.read_signal \
              test/gtest_links/PlatformIOTest.read_signal_agg \
              test/gtest_links/PlatformIOTest.read_signal_override \
              test/gtest_links/PlatformIOTest.sample \
              test/gtest_links/PlatformIOTest.sample_agg \
              test/gtest_links/PlatformIOTest.signal_control_names \
              test/gtest_links/PlatformIOTest.signal_power \
              test/gtest_links/PlatformIOTest.write_control \
              test/gtest_links/PlatformIOTest.write_control_override \
              test/gtest_links/PlatformIOTest.write_control_agg \
              test/gtest_links/PlatformTopoTest.bdx_domain_idx \
              test/gtest_links/PlatformTopoTest.bdx_is_nested_domain \
              test/gtest_links/PlatformTopoTest.bdx_domain_nested \
              test/gtest_links/PlatformTopoTest.bdx_num_domain \
              test/gtest_links/PlatformTopoTest.construction \
              test/gtest_links/PlatformTopoTest.create_cache \
              test/gtest_links/PlatformTopoTest.domain_name_to_type \
              test/gtest_links/PlatformTopoTest.domain_type_to_name \
              test/gtest_links/PlatformTopoTest.hsw_num_domain \
              test/gtest_links/PlatformTopoTest.knl_num_domain \
              test/gtest_links/PlatformTopoTest.no0x_num_domain \
              test/gtest_links/PlatformTopoTest.parse_error \
              test/gtest_links/PlatformTopoTest.ppc_num_domain \
              test/gtest_links/PlatformTopoTest.singleton_construction \
              test/gtest_links/PlatformTopoTest.call_c_wrappers \
              test/gtest_links/PolicyTracerTest.construct_update_destruct \
              test/gtest_links/PolicyTracerTest.format \
              test/gtest_links/PowerBalancerAgentTest.leaf_agent \
              test/gtest_links/PowerBalancerAgentTest.power_balancer_agent \
              test/gtest_links/PowerBalancerAgentTest.tree_agent \
              test/gtest_links/PowerBalancerAgentTest.tree_root_agent \
              test/gtest_links/PowerBalancerAgentTest.enforce_policy \
              test/gtest_links/PowerBalancerTest.balance \
              test/gtest_links/PowerBalancerTest.is_runtime_stable \
              test/gtest_links/PowerBalancerTest.power_cap \
              test/gtest_links/PowerGovernorAgentTest.adjust_platform \
              test/gtest_links/PowerGovernorAgentTest.aggregate_sample \
              test/gtest_links/PowerGovernorAgentTest.enforce_policy \
              test/gtest_links/PowerGovernorAgentTest.split_policy \
              test/gtest_links/PowerGovernorAgentTest.sample_platform \
              test/gtest_links/PowerGovernorAgentTest.trace \
              test/gtest_links/PowerGovernorAgentTest.wait \
              test/gtest_links/PowerGovernorTest.govern \
              test/gtest_links/PowerGovernorTest.govern_max \
              test/gtest_links/PowerGovernorTest.govern_min \
              test/gtest_links/ProfileTableTest.hello \
              test/gtest_links/ProfileTableTest.name_set_fill_long \
              test/gtest_links/ProfileTableTest.name_set_fill_short \
              test/gtest_links/ProfileTableTest.overfill \
              test/gtest_links/ProfileTest.enter_exit \
              test/gtest_links/ProfileTest.epoch \
              test/gtest_links/ProfileTest.progress \
              test/gtest_links/ProfileTest.region \
              test/gtest_links/ProfileTest.shutdown \
              test/gtest_links/ProfileTest.tprof_table \
              test/gtest_links/ProfileTestIntegration.config \
              test/gtest_links/ProfileTestIntegration.cpu_set_size \
              test/gtest_links/ProfileTestIntegration.misconfig_affinity \
              test/gtest_links/ProfileTestIntegration.misconfig_ctl_shmem \
              test/gtest_links/ProfileTestIntegration.misconfig_table_shmem \
              test/gtest_links/ProfileTestIntegration.misconfig_tprof_shmem \
              test/gtest_links/ProfileTracerTest.construct_update_destruct \
              test/gtest_links/ProfileTracerTest.format \
              test/gtest_links/RegionAggregatorTest.epoch_total \
              test/gtest_links/RegionAggregatorTest.sample_total \
              test/gtest_links/ReporterTest.generate \
              test/gtest_links/RuntimeRegulatorTest.all_in_and_out \
              test/gtest_links/RuntimeRegulatorTest.all_reenter \
              test/gtest_links/RuntimeRegulatorTest.check_start_count \
              test/gtest_links/RuntimeRegulatorTest.config_rank_then_workers \
              test/gtest_links/RuntimeRegulatorTest.exceptions \
              test/gtest_links/RuntimeRegulatorTest.one_rank_reenter_and_exit \
              test/gtest_links/SampleRegulatorTest.align_profile \
              test/gtest_links/SampleRegulatorTest.insert_platform \
              test/gtest_links/SampleRegulatorTest.insert_profile \
              test/gtest_links/SchedTest.test_proc_cpuset_0 \
              test/gtest_links/SchedTest.test_proc_cpuset_1 \
              test/gtest_links/SchedTest.test_proc_cpuset_2 \
              test/gtest_links/SchedTest.test_proc_cpuset_3 \
              test/gtest_links/SchedTest.test_proc_cpuset_4 \
              test/gtest_links/SchedTest.test_proc_cpuset_5 \
              test/gtest_links/SchedTest.test_proc_cpuset_6 \
              test/gtest_links/SchedTest.test_proc_cpuset_7 \
              test/gtest_links/SchedTest.test_proc_cpuset_8 \
              test/gtest_links/SharedMemoryTest.fd_check \
              test/gtest_links/SharedMemoryTest.invalid_construction \
              test/gtest_links/SharedMemoryTest.lock_shmem \
              test/gtest_links/SharedMemoryTest.lock_shmem_u \
              test/gtest_links/SharedMemoryTest.share_data \
              test/gtest_links/SharedMemoryTest.share_data_ipc \
              test/gtest_links/ShmemEndpointTest.write_shm_policy \
              test/gtest_links/ShmemEndpointTest.parse_shm_sample \
              test/gtest_links/ShmemEndpointTest.get_agent \
              test/gtest_links/ShmemEndpointTestIntegration.write_shm \
              test/gtest_links/ShmemEndpointTestIntegration.write_read_policy \
              test/gtest_links/ShmemEndpointTestIntegration.write_read_sample \
              test/gtest_links/ShmemEndpointUserTest.parse_shm_policy \
              test/gtest_links/ShmemEndpointUserTest.write_shm_sample \
              test/gtest_links/ShmemEndpointUserTestIntegration.parse_shm \
              test/gtest_links/TimeIOGroupTest.adjust \
              test/gtest_links/TimeIOGroupTest.is_valid \
              test/gtest_links/TimeIOGroupTest.push \
              test/gtest_links/TimeIOGroupTest.read_nothing \
              test/gtest_links/TimeIOGroupTest.read_signal \
              test/gtest_links/TimeIOGroupTest.read_signal_and_batch \
              test/gtest_links/TimeIOGroupTest.sample \
              test/gtest_links/TracerTest.columns \
              test/gtest_links/TracerTest.region_entry_exit \
              test/gtest_links/TracerTest.update_samples \
              test/gtest_links/TreeCommLevelTest.level_rank \
              test/gtest_links/TreeCommLevelTest.receive_down_complete \
              test/gtest_links/TreeCommLevelTest.receive_down_incomplete \
              test/gtest_links/TreeCommLevelTest.receive_up_complete \
              test/gtest_links/TreeCommLevelTest.receive_up_incomplete \
              test/gtest_links/TreeCommLevelTest.send_down \
              test/gtest_links/TreeCommLevelTest.send_up \
              test/gtest_links/TreeCommTest.geometry \
              test/gtest_links/TreeCommTest.geometry_nonroot \
              test/gtest_links/TreeCommTest.overhead_send \
              test/gtest_links/TreeCommTest.send_receive \
              # end

if ENABLE_MPI
GTEST_TESTS += test/gtest_links/MPIInterfaceTest.geopm_api \
               test/gtest_links/MPIInterfaceTest.mpi_api \
               # end
endif

TESTS += $(GTEST_TESTS) \
         copying_headers/test-license \
         # end

EXTRA_DIST += test/InternalProfile.cpp \
              test/InternalProfile.hpp \
              test/MPIInterfaceTest.cpp \
              test/geopm_test.sh \
              test/legacy_whitelist.out \
              test/no_omp_cpu.c \
              test/pmpi_mock.c \
              # end

test_geopm_test_SOURCES = test/AgentFactoryTest.cpp \
                          test/AggTest.cpp \
                          test/ApplicationIOTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/CNLIOGroupTest.cpp \
                          test/CombinedSignalTest.cpp \
                          test/CommMPIImpTest.cpp \
                          test/ControlMessageTest.cpp \
                          test/ControllerTest.cpp \
                          test/CpuinfoIOGroupTest.cpp \
                          test/CSVTest.cpp \
                          test/DebugIOGroupTest.cpp \
                          test/ELFTest.cpp \
                          test/EndpointTest.cpp \
                          test/EnergyEfficientAgentTest.cpp \
                          test/EnergyEfficientRegionTest.cpp \
                          test/EnvironmentTest.cpp \
                          test/EpochRuntimeRegulatorTest.cpp \
                          test/ExceptionTest.cpp \
                          test/FilePolicyTest.cpp \
                          test/FrequencyGovernorTest.cpp \
                          test/FrequencyMapAgentTest.cpp \
                          test/HelperTest.cpp \
                          test/IOGroupTest.cpp \
                          test/MSRIOGroupTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRTest.cpp \
                          test/PolicyStoreImpTest.cpp \
                          test/MockAgent.hpp \
                          test/MockApplicationIO.hpp \
                          test/MockComm.hpp \
                          test/MockControlMessage.hpp \
                          test/MockEndpointUser.hpp \
                          test/MockEnergyEfficientRegion.hpp \
                          test/MockEpochRuntimeRegulator.hpp \
                          test/MockFrequencyGovernor.hpp \
                          test/MockIOGroup.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPlatformTopo.hpp \
                          test/MockPolicyTracer.hpp \
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
                          test/PolicyTracerTest.cpp \
                          test/PowerBalancerAgentTest.cpp \
                          test/PowerBalancerTest.cpp \
                          test/PowerGovernorAgentTest.cpp \
                          test/PowerGovernorTest.cpp \
                          test/ProfileTableTest.cpp \
                          test/ProfileTest.cpp \
                          test/ProfileTracerTest.cpp \
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
