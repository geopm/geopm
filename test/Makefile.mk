#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
              test/gtest_links/ProfileIOSampleTest.hello \
              test/gtest_links/ProfileTableTest.hello \
              test/gtest_links/ProfileTableTest.name_set_fill_short \
              test/gtest_links/ProfileTableTest.name_set_fill_long \
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
              test/gtest_links/BalancingDeciderTest.plugin \
              test/gtest_links/BalancingDeciderTest.name \
              test/gtest_links/BalancingDeciderTest.supported \
              test/gtest_links/BalancingDeciderTest.new_policy_message \
              test/gtest_links/BalancingDeciderTest.update_policy \
              test/gtest_links/GoverningDeciderTest.plugin \
              test/gtest_links/GoverningDeciderTest.decider_is_supported \
              test/gtest_links/GoverningDeciderTest.name \
              test/gtest_links/GoverningDeciderTest.1_socket_under_budget \
              test/gtest_links/GoverningDeciderTest.1_socket_over_budget \
              test/gtest_links/GoverningDeciderTest.2_socket_under_budget \
              test/gtest_links/GoverningDeciderTest.2_socket_over_budget \
              test/gtest_links/CpuinfoIOGroupTest.valid_signals \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info0 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info1 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info2 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info3 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info4 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info5 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_info6 \
              test/gtest_links/CpuinfoIOGroupTest.parse_cpu_freq \
              test/gtest_links/CpuinfoIOGroupTest.plugin \
              test/gtest_links/EnergyEfficientAgentTest.map \
              test/gtest_links/EnergyEfficientAgentTest.name \
              test/gtest_links/EnergyEfficientAgentTest.hint \
              test/gtest_links/EnergyEfficientAgentTest.online_mode \
              test/gtest_links/EfficientFreqDeciderTest.map \
              test/gtest_links/EfficientFreqDeciderTest.decider_is_supported \
              test/gtest_links/EfficientFreqDeciderTest.name \
              test/gtest_links/EfficientFreqDeciderTest.hint \
              test/gtest_links/EfficientFreqDeciderTest.online_mode \
              test/gtest_links/SharedMemoryTest.fd_check \
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
              test/gtest_links/MSRIOTest.read_aligned \
              test/gtest_links/MSRIOTest.read_unaligned \
              test/gtest_links/MSRIOTest.write \
              test/gtest_links/MSRIOTest.read_batch \
              test/gtest_links/MSRIOTest.write_batch \
              test/gtest_links/MSRTest.msr \
              test/gtest_links/MSRTest.msr_overflow \
              test/gtest_links/MSRTest.msr_signal \
              test/gtest_links/MSRTest.msr_control \
              test/gtest_links/EnergyEfficientRegionTest.freq_starts_at_maximum \
              test/gtest_links/EnergyEfficientRegionTest.update_ignores_nan_sample \
              test/gtest_links/EnergyEfficientRegionTest.only_changes_freq_after_enough_samples \
              test/gtest_links/EnergyEfficientRegionTest.freq_does_not_go_below_min \
              test/gtest_links/EnergyEfficientRegionTest.freq_does_not_go_above_max \
              test/gtest_links/EnergyEfficientRegionTest.performance_decreases_freq_steps_back_up \
              test/gtest_links/EnergyEfficientRegionTest.energy_increases_freq_steps_back_up \
              test/gtest_links/EnergyEfficientRegionTest.after_too_many_increase_freq_stays_at_higher \
              test/gtest_links/EfficientFreqRegionTest.freq_starts_at_maximum \
              test/gtest_links/EfficientFreqRegionTest.update_ignores_nan_sample \
              test/gtest_links/EfficientFreqRegionTest.only_changes_freq_after_enough_samples \
              test/gtest_links/EfficientFreqRegionTest.freq_does_not_go_below_min \
              test/gtest_links/EfficientFreqRegionTest.performance_decreases_freq_steps_back_up \
              test/gtest_links/EfficientFreqRegionTest.energy_increases_freq_steps_back_up \
              test/gtest_links/EfficientFreqRegionTest.after_too_many_increase_freq_stays_at_higher \
              test/gtest_links/RuntimeRegulatorTest.exceptions \
              test/gtest_links/RuntimeRegulatorTest.all_in_and_out \
              test/gtest_links/RuntimeRegulatorTest.all_reenter \
              test/gtest_links/RuntimeRegulatorTest.one_rank_reenter_and_exit \
              test/gtest_links/RuntimeRegulatorTest.config_rank_then_workers \
              test/gtest_links/ModelApplicationTest.parse_config_errors \
              test/gtest_links/PlatformTopoTest.hsw_num_domain \
              test/gtest_links/PlatformTopoTest.knl_num_domain \
              test/gtest_links/PlatformTopoTest.bdx_num_domain \
              test/gtest_links/PlatformTopoTest.ppc_num_domain \
              test/gtest_links/PlatformTopoTest.construction \
              test/gtest_links/PlatformTopoTest.singleton_construction \
              test/gtest_links/PlatformTopoTest.bdx_domain_idx \
              test/gtest_links/PlatformTopoTest.bdx_domain_cpus \
              test/gtest_links/PlatformTopoTest.parse_error \
              test/gtest_links/PlatformTopoTest.domain_name_to_type \
              test/gtest_links/PlatformTopoTest.domain_type_to_name \
              test/gtest_links/SingleTreeCommunicatorTest.hello \
              test/gtest_links/TreeCommunicatorTest.hello \
              test/gtest_links/TreeCommunicatorTest.send_policy_down \
              test/gtest_links/TreeCommunicatorTest.send_sample_up \
              test/gtest_links/TimeIOGroupTest.is_valid \
              test/gtest_links/TimeIOGroupTest.push \
              test/gtest_links/TimeIOGroupTest.read_nothing \
              test/gtest_links/TimeIOGroupTest.sample \
              test/gtest_links/TimeIOGroupTest.adjust \
              test/gtest_links/TimeIOGroupTest.read_signal \
              test/gtest_links/TimeIOGroupTest.read_signal_and_batch \
              test/gtest_links/MSRIOGroupTest.supported_cpuid \
              test/gtest_links/MSRIOGroupTest.signal_error \
              test/gtest_links/MSRIOGroupTest.push_signal \
              test/gtest_links/MSRIOGroupTest.sample \
              test/gtest_links/MSRIOGroupTest.sample_raw \
              test/gtest_links/MSRIOGroupTest.read_signal \
              test/gtest_links/MSRIOGroupTest.signal_alias \
              test/gtest_links/MSRIOGroupTest.control_error \
              test/gtest_links/MSRIOGroupTest.push_control \
              test/gtest_links/MSRIOGroupTest.adjust \
              test/gtest_links/MSRIOGroupTest.write_control \
              test/gtest_links/MSRIOGroupTest.control_alias \
              test/gtest_links/MSRIOGroupTest.whitelist \
              test/gtest_links/MSRIOGroupTest.cpuid \
              test/gtest_links/MSRIOGroupTest.register_msr_signal \
              test/gtest_links/MSRIOGroupTest.register_msr_control \
              test/gtest_links/PlatformIOTest.signal_control_names \
              test/gtest_links/PlatformIOTest.domain_type \
              test/gtest_links/PlatformIOTest.push_signal \
              test/gtest_links/PlatformIOTest.signal_power \
              test/gtest_links/PlatformIOTest.push_control \
              test/gtest_links/PlatformIOTest.sample \
              test/gtest_links/PlatformIOTest.adjust \
              test/gtest_links/PlatformIOTest.read_signal \
              test/gtest_links/PlatformIOTest.write_control \
              test/gtest_links/PlatformIOTest.read_signal_override \
              test/gtest_links/ProfileIOGroupTest.is_valid \
              test/gtest_links/ProfileIOGroupTest.domain_type \
              test/gtest_links/ProfileIOGroupTest.invalid_signal \
              test/gtest_links/ProfileIOGroupTest.control \
              test/gtest_links/ProfileIOGroupTest.region_id \
              test/gtest_links/ProfileIOGroupTest.progress \
              test/gtest_links/ProfileIOGroupTest.runtime_sample \
              test/gtest_links/ProfileIOGroupTest.runtime_read_signal \
              test/gtest_links/CombinedSignalTest.sample_sum \
              test/gtest_links/CombinedSignalTest.sample_flat_derivative \
              test/gtest_links/CombinedSignalTest.sample_slope_derivative \
              test/gtest_links/ProfileTestIntegration.config \
              test/gtest_links/ProfileTestIntegration.misconfig_ctl_shmem \
              test/gtest_links/ProfileTestIntegration.misconfig_tprof_shmem \
              test/gtest_links/ProfileTestIntegration.misconfig_table_shmem \
              test/gtest_links/ProfileTestIntegration.misconfig_affinity \
              test/gtest_links/ProfileTest.region \
              test/gtest_links/ProfileTest.enter_exit \
              test/gtest_links/ProfileTest.progress \
              test/gtest_links/ProfileTest.epoch \
              test/gtest_links/ProfileTest.shutdown \
              test/gtest_links/ProfileTest.tprof_table \
              test/gtest_links/TreeCommLevelTest.level_rank \
              test/gtest_links/TreeCommLevelTest.send_up \
              test/gtest_links/TreeCommLevelTest.send_down \
              test/gtest_links/TreeCommLevelTest.receive_up_complete \
              test/gtest_links/TreeCommLevelTest.receive_up_incomplete \
              test/gtest_links/TreeCommLevelTest.receive_down_complete \
              test/gtest_links/TreeCommLevelTest.receive_down_incomplete \
              test/gtest_links/TreeCommTest.geometry \
              test/gtest_links/TreeCommTest.geometry_nonroot \
              test/gtest_links/TreeCommTest.send_receive \
              test/gtest_links/TreeCommTest.overhead_send \
              test/gtest_links/MonitorAgentTest.fixed_signal_list \
              test/gtest_links/MonitorAgentTest.sample_platform \
              test/gtest_links/MonitorAgentTest.descend_nothing \
              test/gtest_links/MonitorAgentTest.ascend_aggregates_signals \
              test/gtest_links/ReporterTest.generate \
              test/gtest_links/KontrollerTest.single_node \
              test/gtest_links/KontrollerTest.two_level_controller_2 \
              test/gtest_links/KontrollerTest.two_level_controller_1 \
              test/gtest_links/KontrollerTest.two_level_controller_0 \
              test/gtest_links/ManagerIOTest.write_json_file \
              test/gtest_links/ManagerIOTest.write_shm \
              test/gtest_links/ManagerIOTest.negative_write_json_file \
              test/gtest_links/ManagerIOTestIntegration.write_shm \
              test/gtest_links/ManagerIOSamplerTest.parse_json_file \
              test/gtest_links/ManagerIOSamplerTest.negative_parse_json_file \
              test/gtest_links/ManagerIOSamplerTest.parse_shm \
              test/gtest_links/ManagerIOSamplerTest.negative_parse_shm \
              test/gtest_links/ManagerIOSamplerTest.negative_shm_setup_mutex \
              test/gtest_links/ManagerIOSamplerTest.negative_bad_files \
              test/gtest_links/ManagerIOSamplerTestIntegration.parse_shm \
              test/gtest_links/TracerTest.columns \
              test/gtest_links/TracerTest.update_samples \
              test/gtest_links/TracerTest.region_entry_exit \
              test/gtest_links/AgentFactoryTest.static_info_monitor \
              test/gtest_links/ApplicationIOTest.passthrough \
              test/gtest_links/KruntimeRegulatorTest.exceptions \
              test/gtest_links/KruntimeRegulatorTest.all_in_and_out \
              test/gtest_links/KruntimeRegulatorTest.all_reenter \
              test/gtest_links/KruntimeRegulatorTest.one_rank_reenter_and_exit \
              test/gtest_links/KruntimeRegulatorTest.config_rank_then_workers \
              test/gtest_links/EpochRuntimeRegulatorTest.invalid_ranks \
              test/gtest_links/EpochRuntimeRegulatorTest.unknown_region \
              test/gtest_links/EpochRuntimeRegulatorTest.rank_enter_exit_trace \
              test/gtest_links/EpochRuntimeRegulatorTest.all_ranks_enter_exit \
              test/gtest_links/EpochRuntimeRegulatorTest.epoch_runtime \
              test/gtest_links/PowerBalancerAgentTest.power_balancer_agent \
              test/gtest_links/PowerBalancerAgentTest.tree_root_agent \
              test/gtest_links/PowerBalancerAgentTest.tree_agent \
              test/gtest_links/PowerBalancerAgentTest.leaf_agent \
              test/gtest_links/PowerBalancerTest.power_cap \
              test/gtest_links/PowerBalancerTest.is_runtime_stable \
              test/gtest_links/PowerBalancerTest.balance \
              test/gtest_links/PowerGovernorTest.govern \
              test/gtest_links/PowerGovernorTest.govern_min \
              test/gtest_links/PowerGovernorTest.govern_max \
              test/gtest_links/PowerGovernorAgentTest.wait \
              test/gtest_links/PowerGovernorAgentTest.sample_platform \
              test/gtest_links/PowerGovernorAgentTest.adjust_platform \
              test/gtest_links/PowerGovernorAgentTest.ascend \
              test/gtest_links/PowerGovernorAgentTest.descend \
              test/gtest_links/RegionAggregatorTest.sample_total \
              test/gtest_links/RegionAggregatorTest.epoch_total \
              # end

if ENABLE_MPI
GTEST_TESTS += test/gtest_links/MPIInterfaceTest.geopm_api \
               test/gtest_links/MPIInterfaceTest.mpi_api \
               # end
endif

TESTS += $(GTEST_TESTS) \
         copying_headers/test-license \
         # end

EXTRA_DIST += test/geopm_test.sh \
              test/MPIInterfaceTest.cpp \
              test/no_omp_cpu.c \
              test/pmpi_mock.c \
              test/default_policy.json \
              test/legacy_whitelist.out \
              test/InternalProfile.cpp \
              test/InternalProfile.hpp \
              # end

test_geopm_test_SOURCES = test/geopm_test.cpp \
                          test/PlatformFactoryTest.cpp \
                          test/PlatformImpTest.cpp \
                          test/PlatformTopologyTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/GlobalPolicyTest.cpp \
                          test/ManagerIOTest.cpp \
                          test/ExceptionTest.cpp \
                          test/ProfileTableTest.cpp \
                          test/SampleRegulatorTest.cpp \
                          test/RegionTest.cpp \
                          test/PolicyTest.cpp \
                          plugin/BalancingDecider.hpp \
                          plugin/BalancingDecider.cpp \
                          plugin/BalancingDeciderRegister.cpp \
                          test/BalancingDeciderTest.cpp \
                          plugin/GoverningDecider.hpp \
                          plugin/GoverningDecider.cpp \
                          plugin/GoverningDeciderRegister.cpp \
                          test/GoverningDeciderTest.cpp \
                          plugin/EfficientFreqDecider.hpp \
                          plugin/EfficientFreqDecider.cpp \
                          plugin/EfficientFreqDeciderRegister.cpp \
                          src/EnergyEfficientAgent.hpp \
                          src/EnergyEfficientAgent.cpp \
                          test/EnergyEfficientAgentTest.cpp \
                          test/MockIOGroup.hpp \
                          test/MockRegion.hpp \
                          test/MockPolicy.hpp \
                          test/MockPowerBalancer.hpp \
                          test/MockPowerGovernor.hpp \
                          test/CpuinfoIOGroupTest.cpp \
                          test/EfficientFreqDeciderTest.cpp \
                          test/MockComm.hpp \
                          test/MockControlMessage.hpp \
                          test/MockEpochRuntimeRegulator.hpp \
                          test/MockProfileTable.hpp \
                          test/MockProfileThreadTable.hpp \
                          test/MockSampleScheduler.hpp \
                          test/MockPlatform.hpp \
                          test/MockProfileSampler.hpp \
                          test/MockGlobalPolicy.hpp \
                          test/MockPlatformImp.hpp \
                          test/MockPlatformTopology.hpp \
                          test/MockSharedMemory.hpp \
                          test/MockSharedMemoryUser.hpp \
                          test/SharedMemoryTest.cpp \
                          test/EnvironmentTest.cpp \
                          test/SchedTest.cpp \
                          test/ControlMessageTest.cpp \
                          test/CommMPIImpTest.cpp \
                          test/PlatformIOTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRTest.cpp \
                          plugin/EfficientFreqRegion.hpp \
                          plugin/EfficientFreqRegion.cpp \
                          test/EfficientFreqRegionTest.cpp \
                          src/EnergyEfficientRegion.hpp \
                          src/EnergyEfficientRegion.cpp \
                          test/EnergyEfficientRegionTest.cpp \
                          test/RuntimeRegulatorTest.cpp \
                          test/ModelApplicationTest.cpp \
                          tutorial/ModelParse.hpp \
                          tutorial/ModelParse.cpp \
                          tutorial/Imbalancer.cpp \
                          test/PlatformTopoTest.cpp \
                          test/TreeCommunicatorTest.cpp \
                          test/TimeIOGroupTest.cpp \
                          test/MSRIOGroupTest.cpp \
                          test/geopm_test.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPlatformTopo.hpp \
                          test/ProfileIOSampleTest.cpp \
                          test/ProfileIOGroupTest.cpp \
                          test/MockProfileIOSample.hpp \
                          test/CombinedSignalTest.cpp \
                          test/MockRuntimeRegulator.hpp \
                          test/ProfileTest.cpp \
                          test/TreeCommLevelTest.cpp \
                          test/TreeCommTest.cpp \
                          test/MockTreeCommLevel.hpp \
                          test/MonitorAgentTest.cpp \
                          test/AgentFactoryTest.cpp \
                          test/ReporterTest.cpp \
                          test/KontrollerTest.cpp \
                          test/MockApplicationIO.hpp \
                          test/MockAgent.hpp \
                          test/MockReporter.hpp \
                          test/MockTracer.hpp \
                          test/MockTreeComm.hpp \
                          test/MockManagerIOSampler.hpp \
                          test/TracerTest.cpp \
                          test/ApplicationIOTest.cpp \
                          test/MockKprofileIOSample.hpp \
                          test/MockProfileIORuntime.hpp \
                          test/KruntimeRegulatorTest.cpp \
                          test/EpochRuntimeRegulatorTest.cpp \
                          test/PowerBalancerAgentTest.cpp \
                          test/PowerBalancerTest.cpp \
                          test/PowerGovernorTest.cpp \
                          test/PowerGovernorAgentTest.cpp \
                          test/RegionAggregatorTest.cpp \
                          test/MockRegionAggregator.hpp \
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
    test_geopm_mpi_test_api_SOURCES = test/geopm_test.cpp \
                                      test/MPIInterfaceTest.cpp \
                                      # end

    test_geopm_mpi_test_api_LDADD = libgtest.a \
                                    libgmock.a \
                                    libgeopmpolicy.la \
                                    # end

    test_geopm_mpi_test_api_LDFLAGS = $(AM_LDFLAGS)
    test_geopm_mpi_test_api_CFLAGS = $(AM_CFLAGS)
    test_geopm_mpi_test_api_CXXFLAGS= $(AM_CXXFLAGS)
if GEOPM_DISABLE_NULL_PTR
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
