#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

check_PROGRAMS += test/geopm_test

if ENABLE_MPI
    check_PROGRAMS += test/geopm_mpi_test_api
endif

GTEST_TESTS = test/gtest_links/AccumulatorTest.empty \
              test/gtest_links/AccumulatorTest.sum_ones \
              test/gtest_links/AccumulatorTest.sum_idx \
              test/gtest_links/AccumulatorTest.avg_ones \
              test/gtest_links/AccumulatorTest.avg_idx_signal \
              test/gtest_links/AdminTest.agent_no_policy \
              test/gtest_links/AdminTest.config_default \
              test/gtest_links/AdminTest.config_override \
              test/gtest_links/AdminTest.dup_config \
              test/gtest_links/AdminTest.dup_keys \
              test/gtest_links/AdminTest.help \
              test/gtest_links/AdminTest.main \
              test/gtest_links/AdminTest.no_options \
              test/gtest_links/AdminTest.positional_args \
              test/gtest_links/AdminTest.print_config \
              test/gtest_links/AdminTest.two_actions \
              test/gtest_links/AdminTest.allowlist \
              test/gtest_links/AgentFactoryTest.static_info_monitor \
              test/gtest_links/AgentFactoryTest.static_info_balancer \
              test/gtest_links/AgentFactoryTest.static_info_governor \
              test/gtest_links/AgentFactoryTest.static_info_frequency_map \
              test/gtest_links/ApplicationIOTest.passthrough \
              test/gtest_links/ApplicationRecordLogTest.bad_shmem \
              test/gtest_links/ApplicationRecordLogTest.get_sizes \
              test/gtest_links/ApplicationRecordLogTest.empty_dump \
              test/gtest_links/ApplicationRecordLogTest.scoped_lock_test \
              test/gtest_links/ApplicationRecordLogTest.one_entry \
              test/gtest_links/ApplicationRecordLogTest.one_exit \
              test/gtest_links/ApplicationRecordLogTest.one_epoch \
              test/gtest_links/ApplicationRecordLogTest.short_region_entry_exit \
              test/gtest_links/ApplicationRecordLogTest.dump_twice \
              test/gtest_links/ApplicationRecordLogTest.dump_within_region \
              test/gtest_links/ApplicationRecordLogTest.overflow_record_table \
              test/gtest_links/ApplicationRecordLogTest.cannot_overflow_region_table \
              test/gtest_links/ApplicationSamplerTest.one_enter_exit \
              test/gtest_links/ApplicationSamplerTest.one_enter_exit_two_ranks \
              test/gtest_links/ApplicationSamplerTest.string_conversion \
              test/gtest_links/ApplicationSamplerTest.short_regions \
              test/gtest_links/ApplicationSamplerTest.with_epoch \
              test/gtest_links/ApplicationSamplerTest.hash \
              test/gtest_links/ApplicationSamplerTest.hint \
              test/gtest_links/ApplicationSamplerTest.hint_time \
              test/gtest_links/ApplicationSamplerTest.cpu_progress \
              test/gtest_links/ApplicationSamplerTest.sampler_cpu \
              test/gtest_links/ApplicationStatusTest.bad_shmem \
              test/gtest_links/ApplicationStatusTest.hash \
              test/gtest_links/ApplicationStatusTest.hints \
              test/gtest_links/ApplicationStatusTest.update_cache \
              test/gtest_links/ApplicationStatusTest.work_progress \
              test/gtest_links/ApplicationStatusTest.wrong_buffer_size \
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
              test/gtest_links/ControllerTest.construct_with_file_policy_and_init_control \
              test/gtest_links/CommNullImpTest.split \
              test/gtest_links/CommNullImpTest.comm_supported \
              test/gtest_links/CommNullImpTest.ranks \
              test/gtest_links/CommNullImpTest.dimension_create \
              test/gtest_links/CommNullImpTest.read_back_memory_writes \
              test/gtest_links/CommNullImpTest.window_creation_and_destruction \
              test/gtest_links/CommNullImpTest.window_lock \
              test/gtest_links/CommNullImpTest.coordinate \
              test/gtest_links/CommNullImpTest.barrier \
              test/gtest_links/CommNullImpTest.broadcast \
              test/gtest_links/CommNullImpTest.test \
              test/gtest_links/CommNullImpTest.reduce_max \
              test/gtest_links/CommNullImpTest.gather \
              test/gtest_links/CommNullImpTest.gatherv \
              test/gtest_links/CommNullImpTest.window_put \
              test/gtest_links/CommNullImpTest.tear_down \
              test/gtest_links/ControllerTest.construct_with_file_policy \
              test/gtest_links/ControllerTest.get_hostnames \
              test/gtest_links/ControllerTest.run_with_no_policy \
              test/gtest_links/ControllerTest.single_node \
              test/gtest_links/ControllerTest.two_level_controller_0 \
              test/gtest_links/ControllerTest.two_level_controller_1 \
              test/gtest_links/ControllerTest.two_level_controller_2 \
              test/gtest_links/CSVTest.buffer \
              test/gtest_links/CSVTest.columns \
              test/gtest_links/CSVTest.header \
              test/gtest_links/CSVTest.negative \
              test/gtest_links/DebugIOGroupTest.is_valid \
              test/gtest_links/DebugIOGroupTest.push \
              test/gtest_links/DebugIOGroupTest.read_signal \
              test/gtest_links/DebugIOGroupTest.register_signal_error \
              test/gtest_links/DebugIOGroupTest.sample \
              test/gtest_links/DenseLayerTest.test_bad_dimensions \
              test/gtest_links/DenseLayerTest.test_inference \
              test/gtest_links/DomainNetMapTest.test_json_parsing \
              test/gtest_links/DomainNetMapTest.test_plumbing \
              test/gtest_links/EditDistEpochRecordFilterTest.one_region_repeated \
              test/gtest_links/EditDistEpochRecordFilterTest.filter_in \
              test/gtest_links/EditDistEpochRecordFilterTest.filter_out \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_a \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_ab \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_abb \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_abcdc \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_ababc \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_abababc \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_add1 \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_add2 \
              test/gtest_links/EditDistEpochRecordFilterTest.pattern_subtract1 \
              test/gtest_links/EditDistEpochRecordFilterTest.fft_small \
              test/gtest_links/EditDistEpochRecordFilterTest.parse_name \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_a \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_ab \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_abb \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_abcdc \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_ababc \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_abababc \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_add1 \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_add2 \
              test/gtest_links/EditDistPeriodicityDetectorTest.pattern_subtract1 \
              test/gtest_links/EditDistPeriodicityDetectorTest.fft_small \
              test/gtest_links/EndpointTest.attach_wait_loop_timeout_throws \
              test/gtest_links/EndpointTest.detach_wait_loop_timeout_throws \
              test/gtest_links/EndpointTest.get_hostnames \
              test/gtest_links/EndpointTest.get_profile_name \
              test/gtest_links/EndpointTest.write_shm_policy \
              test/gtest_links/EndpointTest.parse_shm_sample \
              test/gtest_links/EndpointTest.get_agent \
              test/gtest_links/EndpointTest.stop_wait_loop \
              test/gtest_links/EndpointTest.wait_attach_timeout_0 \
              test/gtest_links/EndpointTest.wait_detach_timeout_0 \
              test/gtest_links/EndpointTestIntegration.read_sample_before_data_exists \
              test/gtest_links/EndpointTest.wait_stops_when_agent_attaches \
              test/gtest_links/EndpointTest.wait_stops_when_agent_detaches \
              test/gtest_links/EndpointTestIntegration.write_shm \
              test/gtest_links/EndpointTestIntegration.write_read_policy \
              test/gtest_links/EndpointTestIntegration.write_read_sample \
              test/gtest_links/EndpointPolicyTracerTest.construct_update_destruct \
              test/gtest_links/EndpointPolicyTracerTest.format \
              test/gtest_links/EndpointUserTest.agent_name_too_long \
              test/gtest_links/EndpointUserTest.attach \
              test/gtest_links/EndpointUserTest.parse_shm_policy \
              test/gtest_links/EndpointUserTest.profile_name_too_long \
              test/gtest_links/EndpointUserTest.write_shm_sample \
              test/gtest_links/EndpointUserTestIntegration.parse_shm \
              test/gtest_links/EnvironmentTest.internal_defaults \
              test/gtest_links/EnvironmentTest.user_only \
              test/gtest_links/EnvironmentTest.user_only_do_profile \
              test/gtest_links/EnvironmentTest.user_only_do_profile_custom \
              test/gtest_links/EnvironmentTest.user_only_do_profile_name \
              test/gtest_links/EnvironmentTest.default_only \
              test/gtest_links/EnvironmentTest.override_only \
              test/gtest_links/EnvironmentTest.default_and_override \
              test/gtest_links/EnvironmentTest.user_default_and_override \
              test/gtest_links/EnvironmentTest.invalid_ctl \
              test/gtest_links/EnvironmentTest.default_endpoint_user_policy \
              test/gtest_links/EnvironmentTest.default_endpoint_user_policy_override_endpoint \
              test/gtest_links/EnvironmentTest.user_policy_and_endpoint \
              test/gtest_links/EnvironmentTest.user_disable_ompt \
              test/gtest_links/EnvironmentTest.record_filter_on \
              test/gtest_links/EnvironmentTest.record_filter_off \
              test/gtest_links/EnvironmentTest.init_control_set \
              test/gtest_links/EnvironmentTest.init_control_unset \
              test/gtest_links/EnvironmentTest.signal_parser \
              test/gtest_links/EpochIOGroupIntegrationTest.read_batch_count \
              test/gtest_links/EpochIOGroupTest.no_controls \
              test/gtest_links/EpochIOGroupTest.read_batch \
              test/gtest_links/EpochIOGroupTest.sample_count \
              test/gtest_links/EpochIOGroupTest.valid_signals \
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
              test/gtest_links/FrequencyGovernorTest.set_domain_type \
              test/gtest_links/FrequencyMapAgentTest.adjust_platform_gpu \
              test/gtest_links/FrequencyMapAgentTest.adjust_platform_map \
              test/gtest_links/FrequencyMapAgentTest.adjust_platform_uncore \
              test/gtest_links/FrequencyMapAgentTest.name \
              test/gtest_links/FrequencyMapAgentTest.enforce_policy_allnan_invalid \
              test/gtest_links/FrequencyMapAgentTest.enforce_policy_bad_size \
              test/gtest_links/FrequencyMapAgentTest.enforce_policy_default_only \
              test/gtest_links/FrequencyMapAgentTest.enforce_policy_default_cpu_only \
              test/gtest_links/FrequencyMapAgentTest.policy_to_json \
              test/gtest_links/FrequencyMapAgentTest.report_both_map_and_set \
              test/gtest_links/FrequencyMapAgentTest.report_default_freq_hash \
              test/gtest_links/FrequencyMapAgentTest.report_hash_freq_map \
              test/gtest_links/FrequencyMapAgentTest.report_neither_map_nor_set \
              test/gtest_links/FrequencyMapAgentTest.split_policy \
              test/gtest_links/FrequencyMapAgentTest.validate_policy \
              test/gtest_links/FrequencyMapAgentTest.validate_policy_nogpu \
              test/gtest_links/InitControlTest.parse_valid_file \
              test/gtest_links/InitControlTest.parse_empty_file \
              test/gtest_links/InitControlTest.parse_empty_file_name \
              test/gtest_links/InitControlTest.throw_bad_input \
              test/gtest_links/InitControlTest.throw_invalid_write \
              test/gtest_links/LocalNeuralNetTest.test_bad_dimensions \
              test/gtest_links/LocalNeuralNetTest.test_inference \
              test/gtest_links/ModelApplicationTest.parse_config_errors \
              test/gtest_links/MonitorAgentTest.policy_names \
              test/gtest_links/MonitorAgentTest.sample_names \
              test/gtest_links/OptionParserTest.get_invalid \
              test/gtest_links/OptionParserTest.parse_errors \
              test/gtest_links/OptionParserTest.add_option_errors \
              test/gtest_links/OptionParserTest.unset_bool_gets_default \
              test/gtest_links/OptionParserTest.set_bool_flag \
              test/gtest_links/OptionParserTest.unset_string_gets_default\
              test/gtest_links/OptionParserTest.set_string_value \
              test/gtest_links/OptionParserTest.positional_args \
              test/gtest_links/OptionParserTest.help \
              test/gtest_links/OptionParserTest.version \
              test/gtest_links/OptionParserTest.complex \
              test/gtest_links/OptionParserTest.compact_short_options \
              test/gtest_links/OptionParserTest.format_help \
              test/gtest_links/PowerBalancerAgentTest.tree_agent \
              test/gtest_links/PowerBalancerAgentTest.tree_root_agent \
              test/gtest_links/PowerBalancerAgentTest.enforce_policy \
              test/gtest_links/PowerBalancerAgentTest.validate_policy \
              test/gtest_links/PowerBalancerTest.balance \
              test/gtest_links/PowerBalancerTest.is_runtime_stable \
              test/gtest_links/PowerBalancerTest.power_cap \
              test/gtest_links/PowerGovernorAgentTest.adjust_platform \
              test/gtest_links/PowerGovernorAgentTest.aggregate_sample \
              test/gtest_links/PowerGovernorAgentTest.enforce_policy \
              test/gtest_links/PowerGovernorAgentTest.split_policy \
              test/gtest_links/PowerGovernorAgentTest.sample_platform \
              test/gtest_links/PowerGovernorAgentTest.trace \
              test/gtest_links/PowerGovernorAgentTest.validate_policy \
              test/gtest_links/PowerGovernorAgentTest.wait \
              test/gtest_links/PowerGovernorTest.govern \
              test/gtest_links/PowerGovernorTest.govern_max \
              test/gtest_links/PowerGovernorTest.govern_min \
              test/gtest_links/ProcessRegionAggregatorTest.entry_exit \
              test/gtest_links/ProcessRegionAggregatorTest.short_region \
              test/gtest_links/ProcessRegionAggregatorTest.multiple_processes \
              test/gtest_links/ProfileIOGroupTest.is_valid \
              test/gtest_links/ProfileIOGroupTest.aliases \
              test/gtest_links/ProfileIOGroupTest.read_signal_region_hash \
              test/gtest_links/ProfileIOGroupTest.read_signal_hint \
              test/gtest_links/ProfileIOGroupTest.read_signal_thread_progress \
              test/gtest_links/ProfileIOGroupTest.read_signal_hint_time \
              test/gtest_links/ProfileIOGroupTest.batch_signal_region_hash \
              test/gtest_links/ProfileIOGroupTest.batch_signal_hint \
              test/gtest_links/ProfileIOGroupTest.batch_signal_thread_progress \
              test/gtest_links/ProfileIOGroupTest.batch_signal_hint_time \
              test/gtest_links/ProfileIOGroupTest.errors \
              test/gtest_links/ProfileTest.enter_exit \
              test/gtest_links/ProfileTest.enter_exit_nested \
              test/gtest_links/ProfileTest.epoch \
              test/gtest_links/ProfileTest.progress_multithread \
              test/gtest_links/ProfileTracerTest.construct_update_destruct \
              test/gtest_links/ProfileTracerTest.format \
              test/gtest_links/ProxyEpochRecordFilterTest.simple_conversion \
              test/gtest_links/ProxyEpochRecordFilterTest.skip_one \
              test/gtest_links/ProxyEpochRecordFilterTest.skip_two_off_one \
              test/gtest_links/ProxyEpochRecordFilterTest.invalid_construct \
              test/gtest_links/ProxyEpochRecordFilterTest.filter_in \
              test/gtest_links/ProxyEpochRecordFilterTest.filter_out \
              test/gtest_links/ProxyEpochRecordFilterTest.parse_name \
              test/gtest_links/ProxyEpochRecordFilterTest.parse_tutorial_2 \
              test/gtest_links/RecordFilterTest.invalid_filter_name \
              test/gtest_links/RecordFilterTest.make_proxy_epoch \
              test/gtest_links/RecordFilterTest.make_edit_distance \
              test/gtest_links/RegionHintRecommenderTest.test_json_parsing \
              test/gtest_links/RegionHintRecommenderTest.test_plumbing \
              test/gtest_links/ReporterTest.generate \
              test/gtest_links/ReporterTest.generate_conditional \
              test/gtest_links/SampleAggregatorTest.epoch_application_total \
              test/gtest_links/SampleAggregatorTest.sample_application \
              test/gtest_links/SampleAggregatorTest.test_sample_before_update \
              test/gtest_links/SchedTest.test_proc_cpuset_0 \
              test/gtest_links/SchedTest.test_proc_cpuset_1 \
              test/gtest_links/SchedTest.test_proc_cpuset_2 \
              test/gtest_links/SchedTest.test_proc_cpuset_3 \
              test/gtest_links/SchedTest.test_proc_cpuset_4 \
              test/gtest_links/SchedTest.test_proc_cpuset_5 \
              test/gtest_links/SchedTest.test_proc_cpuset_6 \
              test/gtest_links/SchedTest.test_proc_cpuset_7 \
              test/gtest_links/SchedTest.test_proc_cpuset_8 \
              test/gtest_links/SSTClosGovernorTest.is_supported \
              test/gtest_links/SSTClosGovernorTest.govern \
              test/gtest_links/SSTClosGovernorTest.enable \
              test/gtest_links/SSTClosGovernorTest.disable \
              test/gtest_links/SSTFrequencyLimitDetectorTest.returns_single_core_limit_by_default \
              test/gtest_links/SSTFrequencyLimitDetectorTest.returns_max_observed_frequency_when_sst_disabled \
              test/gtest_links/SSTFrequencyLimitDetectorTest.detects_nearest_license_level_limit_bucket_0 \
              test/gtest_links/SSTFrequencyLimitDetectorTest.detects_nearest_license_level_limit_bucket_1 \
              test/gtest_links/SSTFrequencyLimitDetectorTest.limits_license_level_search_if_frequency_capped \
              test/gtest_links/TensorMathTest.test_bad_dimensions \
              test/gtest_links/TensorMathTest.test_diff \
              test/gtest_links/TensorMathTest.test_dot \
              test/gtest_links/TensorMathTest.test_mat_prod \
              test/gtest_links/TensorMathTest.test_self_sum \
              test/gtest_links/TensorMathTest.test_self_diff \
              test/gtest_links/TensorMathTest.test_sigmoid \
              test/gtest_links/TensorMathTest.test_sum \
              test/gtest_links/TensorOneDTest.test_copy \
              test/gtest_links/TensorOneDTest.test_diff \
              test/gtest_links/TensorOneDTest.test_equivalent \
              test/gtest_links/TensorOneDTest.test_input \
              test/gtest_links/TensorOneDTest.test_prod \
              test/gtest_links/TensorOneDTest.test_sigmoid \
              test/gtest_links/TensorOneDTest.test_sum \
              test/gtest_links/TensorOneDIntegrationTest.test_bad_dimensions \
              test/gtest_links/TensorOneDIntegrationTest.test_copy \
              test/gtest_links/TensorOneDIntegrationTest.test_diff \
              test/gtest_links/TensorOneDIntegrationTest.test_dot \
              test/gtest_links/TensorOneDIntegrationTest.test_input \
              test/gtest_links/TensorOneDIntegrationTest.test_self_diff \
              test/gtest_links/TensorOneDIntegrationTest.test_self_sum \
              test/gtest_links/TensorOneDIntegrationTest.test_sigmoid \
              test/gtest_links/TensorOneDIntegrationTest.test_sum \
              test/gtest_links/TensorTwoDTest.test_array_overload \
              test/gtest_links/TensorTwoDTest.test_bad_dimensions \
              test/gtest_links/TensorTwoDTest.test_copy_constructor \
              test/gtest_links/TensorTwoDTest.test_copy \
              test/gtest_links/TensorTwoDTest.test_degenerate_size \
              test/gtest_links/TensorTwoDTest.test_empty_weights \
              test/gtest_links/TensorTwoDTest.test_equality \
              test/gtest_links/TensorTwoDTest.test_input \
              test/gtest_links/TensorTwoDTest.test_set_data \
              test/gtest_links/TensorTwoDTest.test_vector_product \
              test/gtest_links/TensorTwoDIntegrationTest.test_bad_dimensions \
              test/gtest_links/TensorTwoDIntegrationTest.test_mat_prod \
              test/gtest_links/TracerTest.columns \
              test/gtest_links/TracerTest.region_entry_exit \
              test/gtest_links/TracerTest.update_samples \
              test/gtest_links/TreeCommLevelTest.level_rank \
              test/gtest_links/TreeCommLevelTest.receive_down_complete \
              test/gtest_links/TreeCommLevelTest.receive_down_incomplete \
              test/gtest_links/TreeCommLevelTest.receive_up_complete \
              test/gtest_links/TreeCommLevelTest.receive_up_incomplete \
              test/gtest_links/TreeCommLevelTest.send_down \
              test/gtest_links/TreeCommLevelTest.send_down_zero_value \
              test/gtest_links/TreeCommLevelTest.send_up \
              test/gtest_links/TreeCommTest.geometry \
              test/gtest_links/TreeCommTest.geometry_nonroot \
              test/gtest_links/TreeCommTest.overhead_send \
              test/gtest_links/TreeCommTest.send_receive \
              test/gtest_links/TRLFrequencyLimitDetectorTest.returns_single_core_limit_by_default \
              test/gtest_links/TRLFrequencyLimitDetectorTest.returns_max_observed_frequency_after_update \
              test/gtest_links/WaiterTest.invalid_strategy_name \
              test/gtest_links/WaiterTest.make_unique \
              test/gtest_links/WaiterTest.reset \
              test/gtest_links/WaiterTest.wait \
              test/gtest_links/ValidateRecordTest.valid_stream \
              test/gtest_links/ValidateRecordTest.process_change \
              test/gtest_links/ValidateRecordTest.entry_exit_paired \
              test/gtest_links/ValidateRecordTest.entry_exit_unpaired \
              test/gtest_links/ValidateRecordTest.double_entry \
              test/gtest_links/ValidateRecordTest.exit_without_entry \
              test/gtest_links/ValidateRecordTest.entry_exit_invalid_hash \
              test/gtest_links/ValidateRecordTest.epoch_count_monotone \
              test/gtest_links/ValidateRecordTest.epoch_count_gap \
              test/gtest_links/ValidateRecordTest.time_monotone \
              # end

if ENABLE_BETA
    GTEST_TESTS += test/gtest_links/CPUActivityAgentTest.name \
                   test/gtest_links/CPUActivityAgentTest.validate_policy \
                   test/gtest_links/CPUActivityAgentTest.adjust_platform_high \
                   test/gtest_links/CPUActivityAgentTest.adjust_platform_medium \
                   test/gtest_links/CPUActivityAgentTest.adjust_platform_low \
                   test/gtest_links/CPUActivityAgentTest.adjust_platform_zero \
                   test/gtest_links/CPUActivityAgentTest.no_mem_constconfig \
                   test/gtest_links/CPUActivityAgentTest.adjust_platform_lower_bound_check \
                   test/gtest_links/CPUActivityAgentTest.adjust_platform_signal_out_of_bounds \
                   test/gtest_links/CPUActivityAgentTest.control_signal_granularity_check \
                   test/gtest_links/CPUActivityAgentTest.invalid_fe_core_low \
                   test/gtest_links/CPUActivityAgentTest.invalid_fe_core_high \
                   test/gtest_links/CPUActivityAgentTest.invalid_fe_uncore_low \
                   test/gtest_links/CPUActivityAgentTest.invalid_fe_uncore_high \
                   test/gtest_links/DaemonTest.get_default_policy \
                   test/gtest_links/DaemonTest.get_profile_policy \
                   test/gtest_links/FFNetAgentTest.adjust_platform_all \
                   test/gtest_links/FFNetAgentTest.adjust_platform_nans \
                   test/gtest_links/FFNetAgentTest.adjust_platform_no_gpu \
                   test/gtest_links/FFNetAgentTest.agent_name \
                   test/gtest_links/FFNetAgentTest.policy_names \
                   test/gtest_links/FFNetAgentTest.sample_platform \
                   test/gtest_links/FFNetAgentTest.sample_platform_no_gpu \
                   test/gtest_links/FFNetAgentTest.trace_names \
                   test/gtest_links/FFNetAgentTest.trace_names_no_gpu \
                   test/gtest_links/FFNetAgentTest.trace_values \
                   test/gtest_links/FFNetAgentTest.trace_values_no_gpu \
                   test/gtest_links/FFNetAgentTest.validate_badphi_policy\
                   test/gtest_links/FFNetAgentTest.validate_badsize_policy \
                   test/gtest_links/FFNetAgentTest.validate_empty_policy \
                   test/gtest_links/FFNetAgentTest.validate_good_policy\
                   test/gtest_links/GPUActivityAgentTest.name \
                   test/gtest_links/GPUActivityAgentTest.validate_policy \
                   test/gtest_links/GPUActivityAgentTest.adjust_platform_high \
                   test/gtest_links/GPUActivityAgentTest.adjust_platform_medium \
                   test/gtest_links/GPUActivityAgentTest.adjust_platform_low \
                   test/gtest_links/GPUActivityAgentTest.adjust_platform_zero \
                   test/gtest_links/GPUActivityAgentTest.adjust_platform_signal_out_of_bounds_high \
                   test/gtest_links/GPUActivityAgentTest.adjust_platform_signal_out_of_bounds_low \
                   test/gtest_links/GPUActivityAgentTest.invalid_fe \
                   test/gtest_links/PolicyStoreImpTest.self_consistent \
                   test/gtest_links/PolicyStoreImpTest.table_precedence \
                   test/gtest_links/PolicyStoreImpTest.update_policy \
                   # end
endif

if ENABLE_MPI
    GTEST_TESTS += test/gtest_links/MPIInterfaceTest.geopm_api \
                   test/gtest_links/MPIInterfaceTest.mpi_api \
                   # end
endif

if ENABLE_OMPT
    GTEST_TESTS += test/gtest_links/ELFTest.symbols_exist \
                   test/gtest_links/ELFTest.symbol_lookup \
                   # end
endif

TESTS_ENVIRONMENT = PYTHON='$(PYTHON)'

TESTS += $(GTEST_TESTS) \
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
                           src/Profile.hpp \
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

# Target for building test programs.
gtest-checkprogs: $(GTEST_TESTS)

PHONY_TARGETS += gtest-checkprogs

$(GTEST_TESTS): test/gtest_links/%:
	mkdir -p test/gtest_links
	rm -f $@
	ln -s $(abs_builddir)/test/geopm_test.sh $@

init-coverage:
	lcov --no-external --capture --initial --directory src --output-file coverage-base-initial.info

coverage: | init-coverage check
	lcov --no-external --capture --directory src --output-file coverage-base.info
	lcov -a coverage-base-initial.info -a coverage-base.info --output-file coverage-base-combined.info
	lcov --remove coverage-base-combined.info "$$(realpath $$(pwd))/src/geopm_pmpi_fortran.c" --output-file coverage-base-combined-filtered.info
	genhtml coverage-base-combined-filtered.info --output-directory coverage-base --legend -t $(VERSION) -f

clean-local-gtest-script-links:
	rm -f test/gtest_links/*

CLEAN_LOCAL_TARGETS += clean-local-gtest-script-links

include test/googletest.mk
