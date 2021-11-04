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

check_PROGRAMS += test/geopm_test

GTEST_TESTS = test/gtest_links/AcceleratorTopoNullTest.default_config \
              test/gtest_links/AggTest.agg_function \
              test/gtest_links/AggTest.function_strings \
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
              test/gtest_links/CircularBufferTest.make_vector_slice \
              test/gtest_links/CNLIOGroupTest.valid_signals \
              test/gtest_links/CNLIOGroupTest.read_signal \
              test/gtest_links/CNLIOGroupTest.push_signal \
              test/gtest_links/CNLIOGroupTest.parse_energy \
              test/gtest_links/CNLIOGroupTest.parse_power \
              test/gtest_links/CombinedSignalTest.sample_flat_derivative \
              test/gtest_links/CombinedSignalTest.sample_max \
              test/gtest_links/CombinedSignalTest.sample_slope_derivative \
              test/gtest_links/CombinedSignalTest.sample_sum \
              test/gtest_links/CombinedSignalTest.sample_difference \
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
              test/gtest_links/CpuinfoIOGroupTest.push_signal \
              test/gtest_links/CpuinfoIOGroupTest.read_signal \
              test/gtest_links/CpuinfoIOGroupTest.valid_signals \
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
              test/gtest_links/DomainControlTest.errors \
              test/gtest_links/DomainControlTest.save_restore \
              test/gtest_links/DomainControlTest.setup_batch \
              test/gtest_links/DomainControlTest.write \
              test/gtest_links/DomainControlTest.write_batch \
              test/gtest_links/ExceptionTest.check_ronn \
              test/gtest_links/ExceptionTest.hello \
              test/gtest_links/ExceptionTest.last_message \
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
              test/gtest_links/LevelZeroAcceleratorTopoTest.no_gpu_config \
              test/gtest_links/LevelZeroAcceleratorTopoTest.four_forty_config \
              test/gtest_links/LevelZeroAcceleratorTopoTest.eight_fiftysix_affinitization_config \
              test/gtest_links/LevelZeroAcceleratorTopoTest.uneven_affinitization_config \
              test/gtest_links/LevelZeroAcceleratorTopoTest.high_cpu_count_config \
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
              test/gtest_links/LevelZeroIOGroupTest.write_control \
              test/gtest_links/LevelZeroIOGroupTest.save_restore \
              test/gtest_links/LevelZeroIOGroupTest.push_control_adjust_write_batch \
              test/gtest_links/LevelZeroIOGroupTest.read_signal_and_batch \
              test/gtest_links/LevelZeroIOGroupTest.read_timestamp_batch \
              test/gtest_links/LevelZeroIOGroupTest.read_timestamp_batch_reverse \
              test/gtest_links/MSRIOGroupTest.adjust \
              test/gtest_links/MSRIOGroupTest.control_error \
              test/gtest_links/MSRIOGroupTest.cpuid \
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
              test/gtest_links/NVMLAcceleratorTopoTest.hpe_sx40_default_config \
              test/gtest_links/NVMLAcceleratorTopoTest.no_gpu_config \
              test/gtest_links/NVMLAcceleratorTopoTest.mutex_affinitization_config \
              test/gtest_links/NVMLAcceleratorTopoTest.equidistant_affinitization_config \
              test/gtest_links/NVMLAcceleratorTopoTest.n1_superset_n_affinitization_config \
              test/gtest_links/NVMLAcceleratorTopoTest.greedbuster_affinitization_config \
              test/gtest_links/NVMLAcceleratorTopoTest.hpe_6500_affinitization_config \
              test/gtest_links/NVMLAcceleratorTopoTest.uneven_affinitization_config \
              test/gtest_links/NVMLAcceleratorTopoTest.high_cpu_count_config \
              test/gtest_links/NVMLAcceleratorTopoTest.high_cpu_count_gaps_config \
              test/gtest_links/NVMLIOGroupTest.read_signal \
              test/gtest_links/NVMLIOGroupTest.read_signal_and_batch \
              test/gtest_links/NVMLIOGroupTest.write_control \
              test/gtest_links/NVMLIOGroupTest.push_control_adjust_write_batch \
              test/gtest_links/NVMLIOGroupTest.error_path \
              test/gtest_links/NVMLIOGroupTest.valid_signals \
              test/gtest_links/PlatformIOTest.adjust \
              test/gtest_links/PlatformIOTest.adjust_agg \
              test/gtest_links/PlatformIOTest.agg_function \
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
              test/gtest_links/PlatformIOTest.signal_behavior \
              test/gtest_links/PlatformIOTest.signal_control_description \
              test/gtest_links/PlatformIOTest.signal_control_names \
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
              test/gtest_links/ServiceIOGroupTest.signal_control_info \
              test/gtest_links/ServiceIOGroupTest.domain_type \
              test/gtest_links/ServiceIOGroupTest.read_signal_behavior \
              test/gtest_links/ServiceIOGroupTest.write_control \
              test/gtest_links/ServiceIOGroupTest.valid_signal_aggregation \
              test/gtest_links/ServiceIOGroupTest.valid_format_function \
              test/gtest_links/ServiceIOGroupTest.push_signal \
              test/gtest_links/ServiceIOGroupTest.push_control \
              test/gtest_links/ServiceIOGroupTest.read_batch \
              test/gtest_links/ServiceIOGroupTest.write_batch \
              test/gtest_links/ServiceIOGroupTest.sample \
              test/gtest_links/ServiceIOGroupTest.adjust \
              test/gtest_links/ServiceProxyTest.platform_get_user_access \
              test/gtest_links/ServiceProxyTest.platform_get_signal_info \
              test/gtest_links/ServiceProxyTest.platform_get_control_info \
              test/gtest_links/ServiceProxyTest.platform_open_session \
              test/gtest_links/ServiceProxyTest.platform_close_session \
              test/gtest_links/ServiceProxyTest.platform_start_batch \
              test/gtest_links/ServiceProxyTest.platform_stop_batch \
              test/gtest_links/ServiceProxyTest.platform_read_signal \
              test/gtest_links/ServiceProxyTest.platform_write_control \
              test/gtest_links/SharedMemoryTest.fd_check \
              test/gtest_links/SharedMemoryTest.invalid_construction \
              test/gtest_links/SharedMemoryTest.lock_shmem \
              test/gtest_links/SharedMemoryTest.lock_shmem_u \
              test/gtest_links/SharedMemoryTest.share_data \
              test/gtest_links/SharedMemoryTest.share_data_ipc \
              test/gtest_links/SharedMemoryTest.default_permissions \
              test/gtest_links/SharedMemoryTest.secure_permissions \
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

test_geopm_test_SOURCES = test/AcceleratorTopoNullTest.cpp \
                          test/AggTest.cpp \
                          test/BatchStatusTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/CNLIOGroupTest.cpp \
                          test/CombinedSignalTest.cpp \
                          test/CpuinfoIOGroupTest.cpp \
                          test/DerivativeSignalTest.cpp \
                          test/DifferenceSignalTest.cpp \
                          test/DomainControlTest.cpp \
                          test/ExceptionTest.cpp \
                          test/geopm_test.cpp \
                          test/geopm_test.hpp \
                          test/geopm_test_helper.cpp \
                          test/HelperTest.cpp \
                          test/IOGroupTest.cpp \
                          test/LevelZeroAcceleratorTopoTest.cpp \
                          test/LevelZeroDevicePoolTest.cpp \
                          test/LevelZeroIOGroupTest.cpp \
                          test/MSRIOGroupTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRFieldControlTest.cpp \
                          test/MSRFieldSignalTest.cpp \
                          test/MockAcceleratorTopo.hpp \
                          test/MockControl.hpp \
                          test/MockIOGroup.hpp \
                          test/MockMSRIO.hpp \
                          test/MockNVMLDevicePool.hpp \
                          test/MockLevelZeroDevicePool.hpp \
                          test/MockLevelZero.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPlatformTopo.cpp \
                          test/MockPlatformTopo.hpp \
                          test/MockSDBus.hpp \
                          test/MockSDBusMessage.hpp \
                          test/MockServiceProxy.hpp \
                          test/MockSharedMemory.hpp \
                          test/MockSignal.hpp \
                          test/MockSSTIO.hpp \
                          test/MockSSTIoctl.hpp \
                          test/NVMLAcceleratorTopoTest.cpp \
                          test/NVMLIOGroupTest.cpp \
                          test/POSIXSignalTest.cpp \
                          test/PlatformIOTest.cpp \
                          test/PlatformTopoTest.cpp \
                          test/RawMSRSignalTest.cpp \
                          test/SharedMemoryTest.cpp \
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
