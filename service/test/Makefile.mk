#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

check_PROGRAMS += test/geopm_test

GTEST_TESTS = test/gtest_links/GPUTopoNullTest.default_config \
              test/gtest_links/AggTest.agg_function \
              test/gtest_links/AggTest.function_strings \
              test/gtest_links/BatchClientTest.read_batch \
              test/gtest_links/BatchClientTest.write_batch \
              test/gtest_links/BatchClientTest.write_batch_wrong_size \
              test/gtest_links/BatchClientTest.write_batch_wrong_size_empty \
              test/gtest_links/BatchClientTest.read_batch_empty \
              test/gtest_links/BatchClientTest.write_batch_empty \
              test/gtest_links/BatchClientTest.create_but_timeout \
              test/gtest_links/BatchClientTest.stop_batch \
              test/gtest_links/BatchServerTest.get_server_pid \
              test/gtest_links/BatchServerTest.get_server_key \
              test/gtest_links/BatchServerTest.stop_batch \
              test/gtest_links/BatchServerTest.stop_batch_exception \
              test/gtest_links/BatchServerTest.run_batch_read \
              test/gtest_links/BatchServerTest.run_batch_read_empty \
              test/gtest_links/BatchServerTest.run_batch_write \
              test/gtest_links/BatchServerTest.run_batch_write_empty \
              test/gtest_links/BatchServerTest.receive_message_terminate \
              test/gtest_links/BatchServerTest.receive_message_default \
              test/gtest_links/BatchServerTest.receive_message_exception \
              test/gtest_links/BatchServerTest.write_message_exception \
              test/gtest_links/BatchServerTest.read_batch_exception \
              test/gtest_links/BatchServerTest.create_shmem \
              test/gtest_links/BatchServerTest.fork_with_setup \
              test/gtest_links/BatchServerTest.fork_with_setup_exception \
              test/gtest_links/BatchServerTest.destructor_exceptions \
              test/gtest_links/BatchServerTest.fork_and_terminate_child \
              test/gtest_links/BatchServerTest.fork_and_terminate_parent \
              test/gtest_links/BatchServerTest.action_sigchld \
              test/gtest_links/BatchServerTest.action_sigchld_error \
              test/gtest_links/BatchServerNameTest.signal_shmem_key \
              test/gtest_links/BatchServerNameTest.control_shmem_key \
              test/gtest_links/BatchStatusTest.client_send_to_server_fifo_expect \
              test/gtest_links/BatchStatusTest.server_send_to_client_fifo_expect \
              test/gtest_links/BatchStatusTest.server_send_to_client_fifo \
              test/gtest_links/BatchStatusTest.both_send_at_once_fifo_expect \
              test/gtest_links/BatchStatusTest.server_and_client_do_nothing \
              test/gtest_links/BatchStatusTest.client_send_to_server_fifo_incorrect_expect \
              test/gtest_links/BatchStatusTest.bad_client_key \
              test/gtest_links/CircularBufferTest.buffer_capacity \
              test/gtest_links/CircularBufferTest.buffer_size \
              test/gtest_links/CircularBufferTest.buffer_values \
              test/gtest_links/CircularBufferTest.buffer_values_negative_indices \
              test/gtest_links/CircularBufferTest.make_vector_slice \
              test/gtest_links/CNLIOGroupTest.valid_signals \
              test/gtest_links/CNLIOGroupTest.read_signal \
              test/gtest_links/CNLIOGroupTest.push_signal \
              test/gtest_links/CNLIOGroupTest.parse_energy \
              test/gtest_links/CNLIOGroupTest.parse_power \
              test/gtest_links/CombinedSignalTest.sample_max \
              test/gtest_links/CombinedSignalTest.sample_sum \
              test/gtest_links/ConstConfigIOGroupTest.input_empty_string \
              test/gtest_links/ConstConfigIOGroupTest.input_empty_json \
              test/gtest_links/ConstConfigIOGroupTest.input_gibberish \
              test/gtest_links/ConstConfigIOGroupTest.input_duplicate_signal \
              test/gtest_links/ConstConfigIOGroupTest.input_missing_properties \
              test/gtest_links/ConstConfigIOGroupTest.input_unexpected_properties \
              test/gtest_links/ConstConfigIOGroupTest.input_capital_properties \
              test/gtest_links/ConstConfigIOGroupTest.input_duplicate_properties \
              test/gtest_links/ConstConfigIOGroupTest.input_empty_domain \
              test/gtest_links/ConstConfigIOGroupTest.input_empty_description \
              test/gtest_links/ConstConfigIOGroupTest.input_empty_units \
              test/gtest_links/ConstConfigIOGroupTest.input_empty_aggregation \
              test/gtest_links/ConstConfigIOGroupTest.input_invalid_domain \
              test/gtest_links/ConstConfigIOGroupTest.input_invalid_units \
              test/gtest_links/ConstConfigIOGroupTest.input_invalid_aggregation \
              test/gtest_links/ConstConfigIOGroupTest.input_incorrect_type \
              test/gtest_links/ConstConfigIOGroupTest.input_array_value_type \
              test/gtest_links/ConstConfigIOGroupTest.input_array_value_empty \
              test/gtest_links/ConstConfigIOGroupTest.valid_json_positive \
              test/gtest_links/ConstConfigIOGroupTest.valid_json_negative \
              test/gtest_links/ConstConfigIOGroupTest.loads_default_config \
              test/gtest_links/ConstConfigIOGroupTest.no_default_config \
              test/gtest_links/CpuinfoIOGroupTest.bad_min_max \
              test/gtest_links/CpuinfoIOGroupTest.bad_sticker \
              test/gtest_links/CpuinfoIOGroupTest.cpuid_sticker_not_supported \
              test/gtest_links/CpuinfoIOGroupTest.plugin \
              test/gtest_links/CpuinfoIOGroupTest.push_signal \
              test/gtest_links/CpuinfoIOGroupTest.read_signal \
              test/gtest_links/CpuinfoIOGroupTest.valid_signals \
              test/gtest_links/DCGMIOGroupTest.read_signal \
              test/gtest_links/DCGMIOGroupTest.read_signal_and_batch \
              test/gtest_links/DCGMIOGroupTest.write_control \
              test/gtest_links/DCGMIOGroupTest.push_control_adjust_write_batch \
              test/gtest_links/DCGMIOGroupTest.error_path \
              test/gtest_links/DCGMIOGroupTest.valid_signals \
              test/gtest_links/DerivativeSignalTest.errors \
              test/gtest_links/DerivativeSignalTest.read_batch_flat \
              test/gtest_links/DerivativeSignalTest.read_batch_first \
              test/gtest_links/DerivativeSignalTest.read_batch_slope_1 \
              test/gtest_links/DerivativeSignalTest.read_batch_slope_2 \
              test/gtest_links/DerivativeSignalTest.read_flat \
              test/gtest_links/DerivativeSignalTest.read_slope_1 \
              test/gtest_links/DerivativeSignalTest.setup_batch \
              test/gtest_links/DifferenceSignalTest.errors \
              test/gtest_links/DifferenceSignalTest.read \
              test/gtest_links/DifferenceSignalTest.read_batch \
              test/gtest_links/DifferenceSignalTest.setup_batch \
              test/gtest_links/RatioSignalTest.errors \
              test/gtest_links/RatioSignalTest.read \
              test/gtest_links/RatioSignalTest.read_div_by_zero \
              test/gtest_links/RatioSignalTest.read_batch \
              test/gtest_links/RatioSignalTest.read_batch_div_by_zero \
              test/gtest_links/RatioSignalTest.setup_batch \
              test/gtest_links/DomainControlTest.errors \
              test/gtest_links/DomainControlTest.save_restore \
              test/gtest_links/DomainControlTest.setup_batch \
              test/gtest_links/DomainControlTest.write \
              test/gtest_links/DomainControlTest.write_batch \
              test/gtest_links/ExceptionTest.check_ronn \
              test/gtest_links/ExceptionTest.hello \
              test/gtest_links/ExceptionTest.last_message \
              test/gtest_links/GEOPMHintTest.check_hint \
              test/gtest_links/HelperTest.string_begins_with \
              test/gtest_links/HelperTest.string_ends_with \
              test/gtest_links/HelperTest.string_join \
              test/gtest_links/HelperTest.string_split \
              test/gtest_links/HelperTest.pid_to \
              test/gtest_links/IOGroupTest.control_names_are_valid \
              test/gtest_links/IOGroupTest.controls_have_descriptions \
              test/gtest_links/IOGroupTest.signal_names_are_valid \
              test/gtest_links/IOGroupTest.signals_have_agg_functions \
              test/gtest_links/IOGroupTest.signals_have_descriptions \
              test/gtest_links/IOGroupTest.signals_have_format_functions \
              test/gtest_links/IOGroupTest.string_to_behavior \
              test/gtest_links/LevelZeroGPUTopoTest.no_gpu_config \
              test/gtest_links/LevelZeroGPUTopoTest.four_forty_config \
              test/gtest_links/LevelZeroGPUTopoTest.eight_fiftysix_affinitization_config \
              test/gtest_links/LevelZeroGPUTopoTest.uneven_affinitization_config \
              test/gtest_links/LevelZeroGPUTopoTest.high_cpu_count_config \
              test/gtest_links/LevelZeroDevicePoolTest.device_count \
              test/gtest_links/LevelZeroDevicePoolTest.subdevice_conversion_and_function \
              test/gtest_links/LevelZeroDevicePoolTest.subdevice_conversion_error \
              test/gtest_links/LevelZeroDevicePoolTest.domain_error \
              test/gtest_links/LevelZeroDevicePoolTest.subdevice_range_check \
              test/gtest_links/LevelZeroDevicePoolTest.device_range_check \
              test/gtest_links/LevelZeroDevicePoolTest.device_function_check \
              test/gtest_links/LevelZeroIOGroupTest.valid_signals \
              test/gtest_links/LevelZeroIOGroupTest.read_signal \
              test/gtest_links/LevelZeroIOGroupTest.error_path \
              test/gtest_links/LevelZeroIOGroupTest.signal_and_control_trimming \
              test/gtest_links/LevelZeroIOGroupTest.write_control \
              test/gtest_links/LevelZeroIOGroupTest.save_restore \
              test/gtest_links/LevelZeroIOGroupTest.push_control_adjust_write_batch \
              test/gtest_links/LevelZeroIOGroupTest.read_signal_and_batch \
              test/gtest_links/LevelZeroIOGroupTest.read_timestamp_batch \
              test/gtest_links/LevelZeroIOGroupTest.read_timestamp_batch_reverse \
              test/gtest_links/LevelZeroIOGroupTest.save_restore_control \
              test/gtest_links/MSRIOGroupTest.adjust \
              test/gtest_links/MSRIOGroupTest.control_error \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs_error_fields \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs_error_msrs \
              test/gtest_links/MSRIOGroupTest.parse_json_msrs_error_top_level \
              test/gtest_links/MSRIOGroupTest.push_control \
              test/gtest_links/MSRIOGroupTest.push_signal \
              test/gtest_links/MSRIOGroupTest.push_signal_temperature \
              test/gtest_links/MSRIOGroupTest.read_signal_counter \
              test/gtest_links/MSRIOGroupTest.read_signal_energy \
              test/gtest_links/MSRIOGroupTest.read_signal_frequency \
              test/gtest_links/MSRIOGroupTest.read_signal_power \
              test/gtest_links/MSRIOGroupTest.read_signal_scalability \
              test/gtest_links/MSRIOGroupTest.read_signal_temperature \
              test/gtest_links/MSRIOGroupTest.sample \
              test/gtest_links/MSRIOGroupTest.sample_raw \
              test/gtest_links/MSRIOGroupTest.signal_error \
              test/gtest_links/MSRIOGroupTest.supported_cpuid \
              test/gtest_links/MSRIOGroupTest.valid_signal_aggregation \
              test/gtest_links/MSRIOGroupTest.valid_signal_domains \
              test/gtest_links/MSRIOGroupTest.valid_signal_format \
              test/gtest_links/MSRIOGroupTest.valid_signal_names \
              test/gtest_links/MSRIOGroupTest.allowlist \
              test/gtest_links/MSRIOGroupTest.write_control \
              test/gtest_links/MSRIOGroupTest.batch_calls_no_push \
              test/gtest_links/MSRIOGroupTest.save_restore_control \
              test/gtest_links/MSRIOGroupTest.turbo_ratio_limit_writability \
              test/gtest_links/MSRIOTest.read_aligned \
              test/gtest_links/MSRIOTest.read_batch \
              test/gtest_links/MSRIOTest.read_unaligned \
              test/gtest_links/MSRIOTest.write \
              test/gtest_links/MSRIOTest.write_batch \
              test/gtest_links/MSRFieldControlTest.errors \
              test/gtest_links/MSRFieldControlTest.save_restore \
              test/gtest_links/MSRFieldControlTest.setup_batch \
              test/gtest_links/MSRFieldControlTest.write_batch_7_bit_float \
              test/gtest_links/MSRFieldControlTest.write_7_bit_float \
              test/gtest_links/MSRFieldControlTest.write_batch_log_half \
              test/gtest_links/MSRFieldControlTest.write_log_half \
              test/gtest_links/MSRFieldControlTest.write_batch_scale \
              test/gtest_links/MSRFieldControlTest.write_scale \
              test/gtest_links/MSRFieldSignalTest.read_batch_7_bit_float \
              test/gtest_links/MSRFieldSignalTest.read_batch_log_half \
              test/gtest_links/MSRFieldSignalTest.read_batch_overflow \
              test/gtest_links/MSRFieldSignalTest.read_batch_scale \
              test/gtest_links/MSRFieldSignalTest.read_7_bit_float \
              test/gtest_links/MSRFieldSignalTest.read_log_half \
              test/gtest_links/MSRFieldSignalTest.read_overflow \
              test/gtest_links/MSRFieldSignalTest.read_scale \
              test/gtest_links/MSRFieldSignalTest.real_counter \
              test/gtest_links/MSRFieldSignalTest.setup_batch \
              test/gtest_links/MSRFieldSignalTest.errors \
              test/gtest_links/NVMLGPUTopoTest.hpe_sx40_default_config \
              test/gtest_links/NVMLGPUTopoTest.no_gpu_config \
              test/gtest_links/NVMLGPUTopoTest.mutex_affinitization_config \
              test/gtest_links/NVMLGPUTopoTest.equidistant_affinitization_config \
              test/gtest_links/NVMLGPUTopoTest.n1_superset_n_affinitization_config \
              test/gtest_links/NVMLGPUTopoTest.greedbuster_affinitization_config \
              test/gtest_links/NVMLGPUTopoTest.hpe_6500_affinitization_config \
              test/gtest_links/NVMLGPUTopoTest.uneven_affinitization_config \
              test/gtest_links/NVMLGPUTopoTest.high_cpu_count_config \
              test/gtest_links/NVMLGPUTopoTest.high_cpu_count_gaps_config \
              test/gtest_links/NVMLIOGroupTest.read_signal \
              test/gtest_links/NVMLIOGroupTest.read_signal_and_batch \
              test/gtest_links/NVMLIOGroupTest.write_control \
              test/gtest_links/NVMLIOGroupTest.push_control_adjust_write_batch \
              test/gtest_links/NVMLIOGroupTest.error_path \
              test/gtest_links/NVMLIOGroupTest.valid_signals \
              test/gtest_links/NVMLIOGroupTest.signal_and_control_trimming \
              test/gtest_links/PlatformIOTest.adjust \
              test/gtest_links/PlatformIOTest.adjust_agg \
              test/gtest_links/PlatformIOTest.adjust_agg_sum \
              test/gtest_links/PlatformIOTest.agg_function \
              test/gtest_links/PlatformIOTest.domain_type \
              test/gtest_links/PlatformIOTest.push_control \
              test/gtest_links/PlatformIOTest.push_control_agg \
              test/gtest_links/PlatformIOTest.push_control_iogroup_fallback \
              test/gtest_links/PlatformIOTest.push_control_iogroup_fallback_domain_change \
              test/gtest_links/PlatformIOTest.push_signal \
              test/gtest_links/PlatformIOTest.push_signal_iogroup_fallback \
              test/gtest_links/PlatformIOTest.push_signal_iogroup_fallback_domain_change \
              test/gtest_links/PlatformIOTest.push_signal_agg \
              test/gtest_links/PlatformIOTest.read_signal \
              test/gtest_links/PlatformIOTest.read_signal_agg \
              test/gtest_links/PlatformIOTest.read_signal_iogroup_fallback_domain_change \
              test/gtest_links/PlatformIOTest.read_signal_iogroup_fallback \
              test/gtest_links/PlatformIOTest.read_signal_override \
              test/gtest_links/PlatformIOTest.sample \
              test/gtest_links/PlatformIOTest.sample_not_active \
              test/gtest_links/PlatformIOTest.sample_agg \
              test/gtest_links/PlatformIOTest.save_restore \
              test/gtest_links/PlatformIOTest.signal_behavior \
              test/gtest_links/PlatformIOTest.signal_control_description \
              test/gtest_links/PlatformIOTest.signal_control_names \
              test/gtest_links/PlatformIOTest.write_control \
              test/gtest_links/PlatformIOTest.write_control_iogroup_fallback_domain_change \
              test/gtest_links/PlatformIOTest.write_control_iogroup_fallback \
              test/gtest_links/PlatformIOTest.write_control_override \
              test/gtest_links/PlatformIOTest.write_control_agg \
              test/gtest_links/PlatformIOTest.write_control_agg_sum \
              test/gtest_links/PlatformTopoTest.bdx_domain_idx \
              test/gtest_links/PlatformTopoTest.bdx_is_nested_domain \
              test/gtest_links/PlatformTopoTest.bdx_domain_nested \
              test/gtest_links/PlatformTopoTest.bdx_num_domain \
              test/gtest_links/PlatformTopoTest.call_c_wrappers \
              test/gtest_links/PlatformTopoTest.check_file_bad_perms \
              test/gtest_links/PlatformTopoTest.check_file_too_old \
              test/gtest_links/PlatformTopoTest.construction \
              test/gtest_links/PlatformTopoTest.create_cache \
              test/gtest_links/PlatformTopoTest.domain_name_to_type \
              test/gtest_links/PlatformTopoTest.domain_type_to_name \
              test/gtest_links/PlatformTopoTest.hsw_num_domain \
              test/gtest_links/PlatformTopoTest.knl_num_domain \
              test/gtest_links/PlatformTopoTest.no0x_num_domain \
              test/gtest_links/PlatformTopoTest.no_numa_num_domain \
              test/gtest_links/PlatformTopoTest.parse_error \
              test/gtest_links/PlatformTopoTest.ppc_num_domain \
              test/gtest_links/PlatformTopoTest.singleton_construction \
              test/gtest_links/POSIXSignalTest.make_sigset_correct \
              test/gtest_links/POSIXSignalTest.make_sigset_EINVAL \
              test/gtest_links/POSIXSignalTest.make_sigset_zeroed \
              test/gtest_links/POSIXSignalTest.reduce_info \
              test/gtest_links/POSIXSignalTest.sig_timed_wait_EAGAIN \
              test/gtest_links/POSIXSignalTest.sig_timed_wait_EINVAL \
              test/gtest_links/POSIXSignalTest.sig_queue_EINVAL \
              test/gtest_links/POSIXSignalTest.sig_queue_ESRCH \
              test/gtest_links/POSIXSignalTest.sig_queue_EPERM \
              test/gtest_links/POSIXSignalTest.sig_action_EINVAL \
              test/gtest_links/POSIXSignalTest.sig_proc_mask_SIG_SETMASK \
              test/gtest_links/POSIXSignalTest.sig_proc_mask_SIG_BLOCK \
              test/gtest_links/POSIXSignalTest.sig_proc_mask_SIG_UNBLOCK \
              test/gtest_links/POSIXSignalTest.sig_proc_mask_EINVAL \
              test/gtest_links/POSIXSignalTest.sig_suspend_EFAULT \
              test/gtest_links/RawMSRSignalTest.errors \
              test/gtest_links/RawMSRSignalTest.read \
              test/gtest_links/RawMSRSignalTest.read_batch \
              test/gtest_links/RawMSRSignalTest.setup_batch \
              test/gtest_links/SaveControlTest.static_json \
              test/gtest_links/SaveControlTest.static_settings \
              test/gtest_links/SaveControlTest.make_from_struct \
              test/gtest_links/SaveControlTest.make_from_string \
              test/gtest_links/SaveControlTest.make_from_io_group \
              test/gtest_links/SaveControlTest.write_file \
              test/gtest_links/SaveControlTest.bad_json \
              test/gtest_links/ServiceIOGroupTest.signal_control_info \
              test/gtest_links/ServiceIOGroupTest.domain_type \
              test/gtest_links/ServiceIOGroupTest.read_signal_behavior \
              test/gtest_links/ServiceIOGroupTest.read_signal_exception \
              test/gtest_links/ServiceIOGroupTest.write_control \
              test/gtest_links/ServiceIOGroupTest.write_control_exception \
              test/gtest_links/ServiceIOGroupTest.valid_signal_aggregation \
              test/gtest_links/ServiceIOGroupTest.valid_format_function \
              test/gtest_links/ServiceIOGroupTest.push_signal \
              test/gtest_links/ServiceIOGroupTest.push_control \
              test/gtest_links/ServiceIOGroupTest.read_batch \
              test/gtest_links/ServiceIOGroupTest.write_batch \
              test/gtest_links/ServiceIOGroupTest.save_control \
              test/gtest_links/ServiceIOGroupTest.restore_control \
              test/gtest_links/ServiceProxyTest.platform_get_user_access \
              test/gtest_links/ServiceProxyTest.platform_get_signal_info \
              test/gtest_links/ServiceProxyTest.platform_get_control_info \
              test/gtest_links/ServiceProxyTest.platform_open_session \
              test/gtest_links/ServiceProxyTest.platform_close_session \
              test/gtest_links/ServiceProxyTest.platform_start_batch \
              test/gtest_links/ServiceProxyTest.platform_stop_batch \
              test/gtest_links/ServiceProxyTest.platform_read_signal \
              test/gtest_links/ServiceProxyTest.platform_write_control \
              test/gtest_links/SharedMemoryTest.fd_check_shm \
              test/gtest_links/SharedMemoryTest.fd_check_file \
              test/gtest_links/SharedMemoryTest.invalid_construction \
              test/gtest_links/SharedMemoryTest.lock_shmem_shm \
              test/gtest_links/SharedMemoryTest.lock_shmem_file \
              test/gtest_links/SharedMemoryTest.lock_shmem_u_shm \
              test/gtest_links/SharedMemoryTest.lock_shmem_u_file \
              test/gtest_links/SharedMemoryTest.share_data_shm \
              test/gtest_links/SharedMemoryTest.share_data_file \
              test/gtest_links/SharedMemoryTest.share_data_ipc_shm \
              test/gtest_links/SharedMemoryTest.share_data_ipc_file \
              test/gtest_links/SharedMemoryTest.default_permissions_shm \
              test/gtest_links/SharedMemoryTest.default_permissions_file \
              test/gtest_links/SharedMemoryTest.secure_permissions_shm \
              test/gtest_links/SharedMemoryTest.secure_permissions_file \
              test/gtest_links/SSTControlTest.mailbox_adjust_batch \
              test/gtest_links/SSTControlTest.mmio_adjust_batch \
              test/gtest_links/SSTControlTest.save_restore_mmio \
              test/gtest_links/SSTControlTest.save_restore_mbox \
              test/gtest_links/SSTIOGroupTest.adjust_mbox_control \
              test/gtest_links/SSTIOGroupTest.adjust_mmio_control \
              test/gtest_links/SSTIOGroupTest.error_in_save_removes_control \
              test/gtest_links/SSTIOGroupTest.sample_mbox_control \
              test/gtest_links/SSTIOGroupTest.sample_mbox_signal \
              test/gtest_links/SSTIOGroupTest.sample_mmio_percore_control \
              test/gtest_links/SSTIOGroupTest.valid_control_domains \
              test/gtest_links/SSTIOGroupTest.valid_control_names \
              test/gtest_links/SSTIOGroupTest.valid_signal_domains \
              test/gtest_links/SSTIOGroupTest.valid_signal_names \
              test/gtest_links/SSTIOGroupTest.save_restore_control \
              test/gtest_links/SSTIOGroupTest.enable_sst_tf_implies_enable_sst_cp_write_once \
              test/gtest_links/SSTIOGroupTest.disable_sst_cp_implies_disable_sst_tf_write_once \
              test/gtest_links/SSTIOGroupTest.restored_controls_follow_ordered_dependencies_disabled \
              test/gtest_links/SSTIOGroupTest.restored_controls_follow_ordered_dependencies_enabled \
              test/gtest_links/SSTSignalTest.mailbox_read_batch \
              test/gtest_links/SSTSignalTest.mmio_read_batch \
              test/gtest_links/SSTIOTest.mbox_batch_reads \
              test/gtest_links/SSTIOTest.mmio_batch_reads \
              test/gtest_links/SSTIOTest.mbox_batch_writes \
              test/gtest_links/SSTIOTest.mmio_batch_writes \
              test/gtest_links/SSTIOTest.sample_batched_reads \
              test/gtest_links/SSTIOTest.adjust_batched_writes \
              test/gtest_links/SSTIOTest.read_mbox_once \
              test/gtest_links/SSTIOTest.read_mmio_once \
              test/gtest_links/SSTIOTest.write_mbox_once \
              test/gtest_links/SSTIOTest.write_mmio_once \
              test/gtest_links/SSTIOTest.get_punit_from_cpu \
              test/gtest_links/TimeIOGroupTest.adjust \
              test/gtest_links/TimeIOGroupTest.is_valid \
              test/gtest_links/TimeIOGroupTest.push \
              test/gtest_links/TimeIOGroupTest.read_nothing \
              test/gtest_links/TimeIOGroupTest.read_signal \
              test/gtest_links/TimeIOGroupTest.read_signal_and_batch \
              test/gtest_links/TimeIOGroupTest.sample \
              # end

TESTS_ENVIRONMENT = PYTHON='$(PYTHON)'

TESTS += $(GTEST_TESTS) \
         # end

EXTRA_DIST += test/legacy_allowlist.out \
              test/geopm_test.sh \
              # end

test_geopm_test_SOURCES = test/GPUTopoNullTest.cpp \
                          test/AggTest.cpp \
                          test/BatchClientTest.cpp \
                          test/BatchServerTest.cpp \
                          test/BatchStatusTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/CNLIOGroupTest.cpp \
                          test/CombinedSignalTest.cpp \
                          test/ConstConfigIOGroupTest.cpp \
                          test/CpuinfoIOGroupTest.cpp \
                          test/DCGMIOGroupTest.cpp \
                          test/DerivativeSignalTest.cpp \
                          test/DifferenceSignalTest.cpp \
                          test/RatioSignalTest.cpp \
                          test/DomainControlTest.cpp \
                          test/ExceptionTest.cpp \
                          test/geopm_test.cpp \
                          test/geopm_test.hpp \
                          test/geopm_test_helper.cpp \
                          test/GEOPMHintTest.cpp \
                          test/HelperTest.cpp \
                          test/IOGroupTest.cpp \
                          test/LevelZeroGPUTopoTest.cpp \
                          test/LevelZeroDevicePoolTest.cpp \
                          test/LevelZeroIOGroupTest.cpp \
                          test/MSRIOGroupTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRFieldControlTest.cpp \
                          test/MSRFieldSignalTest.cpp \
                          test/MockGPUTopo.hpp \
                          test/MockBatchClient.hpp \
                          test/MockBatchStatus.hpp \
                          test/MockControl.hpp \
                          test/MockDCGMDevicePool.hpp \
                          test/MockIOGroup.hpp \
                          test/MockMSRIO.hpp \
                          test/MockNVMLDevicePool.hpp \
                          test/MockLevelZeroDevicePool.hpp \
                          test/MockLevelZero.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPlatformTopo.cpp \
                          test/MockPlatformTopo.hpp \
                          test/MockSaveControl.hpp \
                          test/MockSDBus.hpp \
                          test/MockSDBusMessage.hpp \
                          test/MockServiceProxy.hpp \
                          test/MockSharedMemory.hpp \
                          test/MockSignal.hpp \
                          test/MockSSTIO.hpp \
                          test/MockSSTIoctl.hpp \
                          test/MockPOSIXSignal.hpp \
                          test/NVMLGPUTopoTest.cpp \
                          test/NVMLIOGroupTest.cpp \
                          test/POSIXSignalTest.cpp \
                          test/PlatformIOTest.cpp \
                          test/PlatformTopoTest.cpp \
                          test/RawMSRSignalTest.cpp \
                          test/SharedMemoryTest.cpp \
                          test/SaveControlTest.cpp \
                          test/ServiceIOGroupTest.cpp \
                          test/ServiceProxyTest.cpp \
                          test/SSTControlTest.cpp \
                          test/SSTIOGroupTest.cpp \
                          test/SSTSignalTest.cpp \
                          test/SSTIOTest.cpp \
                          test/TimeIOGroupTest.cpp \
                          # end

test_geopm_test_LDADD = libgeopmd.la \
                        libgmock.a \
                        libgtest.a \
                        # end

test_geopm_test_CPPFLAGS = $(AM_CPPFLAGS) -Iplugin
test_geopm_test_CFLAGS = $(AM_CFLAGS)
test_geopm_test_CXXFLAGS = $(AM_CXXFLAGS)

# Target for building test programs.
gtest-checkprogs: $(GTEST_TESTS)

PHONY_TARGETS += gtest-checkprogs

$(GTEST_TESTS): test/gtest_links/%:
	mkdir -p test/gtest_links
	rm -f $@
	ln -s $(abs_srcdir)/test/geopm_test.sh $@

init-coverage:
	lcov --no-external --capture --initial --directory src --output-file coverage-service-initial.info

coverage: | init-coverage check
	lcov --no-external --capture --directory src --output-file coverage-service.info
	lcov -a coverage-service-initial.info -a coverage-service.info --output-file coverage-service-combined.info
	genhtml coverage-service-combined.info --output-directory coverage-service --legend -t $(VERSION) -f

clean-local-gtest-script-links:
	rm -f test/gtest_links/*

CLEAN_LOCAL_TARGETS += clean-local-gtest-script-links

include test/googletest.mk
