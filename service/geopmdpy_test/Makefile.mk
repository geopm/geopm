#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += geopmdpy_test/__init__.py \
              geopmdpy_test/__main__.py \
              geopmdpy_test/geopmdpy_test.sh \
              geopmdpy_test/TestAccess.py \
              geopmdpy_test/TestPlatformService.py \
              geopmdpy_test/TestDBusXML.py \
              geopmdpy_test/TestError.py \
              geopmdpy_test/TestPIO.py \
              geopmdpy_test/TestTopo.py \
              geopmdpy_test/TestRequestQueue.py \
              geopmdpy_test/TestSession.py \
              geopmdpy_test/TestTimedLoop.py \
              geopmdpy_test/TestController.py \
              geopmdpy_test/TestActiveSessions.py \
              geopmdpy_test/TestSecureFiles.py \
              geopmdpy_test/TestAccessLists.py \
              geopmdpy_test/TestWriteLock.py \
              geopmdpy_test/TestMSRDataFiles.py \
              # end

GEOPMDPY_TESTS = geopmdpy_test/pytest_links/TestAccessLists.test__read_allowed_invalid \
                 geopmdpy_test/pytest_links/TestAccessLists.test_config_parser \
                 geopmdpy_test/pytest_links/TestAccessLists.test_existing_dir_perms_wrong \
                 geopmdpy_test/pytest_links/TestAccessLists.test_existing_dir_perms_ok \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_group_access_default \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_group_access_empty \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_group_access_invalid \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_group_access_named \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_access_default \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_access_empty \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_access_invalid \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_access_root \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_access_valid \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_groups_current \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_user_groups_invalid \
                 geopmdpy_test/pytest_links/TestAccessLists.test_set_group_access_empty \
                 geopmdpy_test/pytest_links/TestAccessLists.test_set_group_access_named \
                 geopmdpy_test/pytest_links/TestAccessLists.test_get_names \
                 geopmdpy_test/pytest_links/TestPlatformService.test_close_session_invalid \
                 geopmdpy_test/pytest_links/TestPlatformService.test_close_session_read \
                 geopmdpy_test/pytest_links/TestPlatformService.test_close_session_write \
                 geopmdpy_test/pytest_links/TestPlatformService.test_get_control_info \
                 geopmdpy_test/pytest_links/TestPlatformService.test_get_signal_info \
                 geopmdpy_test/pytest_links/TestPlatformService.test_lock_control \
                 geopmdpy_test/pytest_links/TestPlatformService.test_open_session \
                 geopmdpy_test/pytest_links/TestPlatformService.test_open_session_twice \
                 geopmdpy_test/pytest_links/TestPlatformService.test_read_signal \
                 geopmdpy_test/pytest_links/TestPlatformService.test_read_signal_invalid \
                 geopmdpy_test/pytest_links/TestPlatformService.test_start_batch \
                 geopmdpy_test/pytest_links/TestPlatformService.test_start_batch_invalid \
                 geopmdpy_test/pytest_links/TestPlatformService.test_start_batch_write_blocked \
                 geopmdpy_test/pytest_links/TestPlatformService.test_stop_batch_invalid \
                 geopmdpy_test/pytest_links/TestPlatformService.test_unlock_control \
                 geopmdpy_test/pytest_links/TestPlatformService.test_write_control \
                 geopmdpy_test/pytest_links/TestPlatformService.test_write_control_invalid \
                 geopmdpy_test/pytest_links/TestPlatformService.test_get_cache \
                 geopmdpy_test/pytest_links/TestDBusXML.test_xml_parse_no_doc \
                 geopmdpy_test/pytest_links/TestDBusXML.test_xml_parse_with_doc \
                 geopmdpy_test/pytest_links/TestError.test_error_message \
                 geopmdpy_test/pytest_links/TestPIO.test_domain_name \
                 geopmdpy_test/pytest_links/TestPIO.test_signal_names \
                 geopmdpy_test/pytest_links/TestPIO.test_control_names \
                 geopmdpy_test/pytest_links/TestPIO.test_read_signal \
                 geopmdpy_test/pytest_links/TestPIO.test_write_control \
                 geopmdpy_test/pytest_links/TestTopo.test_num_domain \
                 geopmdpy_test/pytest_links/TestTopo.test_domain_domain_nested \
                 geopmdpy_test/pytest_links/TestTopo.test_domain_name_type \
                 geopmdpy_test/pytest_links/TestAccess.test_signals_query \
                 geopmdpy_test/pytest_links/TestAccess.test_controls_query \
                 geopmdpy_test/pytest_links/TestAccess.test_default_signals_query \
                 geopmdpy_test/pytest_links/TestAccess.test_default_controls_query \
                 geopmdpy_test/pytest_links/TestAccess.test_group_signals_query \
                 geopmdpy_test/pytest_links/TestAccess.test_group_controls_query \
                 geopmdpy_test/pytest_links/TestAccess.test_all_signals_query \
                 geopmdpy_test/pytest_links/TestAccess.test_all_controls_query \
                 geopmdpy_test/pytest_links/TestAccess.test_write_default_signals \
                 geopmdpy_test/pytest_links/TestAccess.test_write_invalid_controls \
                 geopmdpy_test/pytest_links/TestAccess.test_write_invalid_signals \
                 geopmdpy_test/pytest_links/TestAccess.test_write_default_controls \
                 geopmdpy_test/pytest_links/TestAccess.test_write_group_signals \
                 geopmdpy_test/pytest_links/TestAccess.test_write_group_controls \
                 geopmdpy_test/pytest_links/TestAccess.test_write_invalid_signals_force \
                 geopmdpy_test/pytest_links/TestAccess.test_write_signals_dry_run \
                 geopmdpy_test/pytest_links/TestAccess.test_write_invalid_signals_dry_run \
                 geopmdpy_test/pytest_links/TestAccess.test_edit_signals \
                 geopmdpy_test/pytest_links/TestAccess.test_delete_default_signals \
                 geopmdpy_test/pytest_links/TestRequestQueue.test_read_request_queue \
                 geopmdpy_test/pytest_links/TestRequestQueue.test_read_request_queue_invalid \
                 geopmdpy_test/pytest_links/TestRequestQueue.test_request_queue_invalid \
                 geopmdpy_test/pytest_links/TestSession.test_check_read_args \
                 geopmdpy_test/pytest_links/TestSession.test_format_signals \
                 geopmdpy_test/pytest_links/TestSession.test_format_signals_invalid \
                 geopmdpy_test/pytest_links/TestSession.test_run \
                 geopmdpy_test/pytest_links/TestSession.test_run_read \
                 geopmdpy_test/pytest_links/TestController.test_agent \
                 geopmdpy_test/pytest_links/TestController.test_controller_construction \
                 geopmdpy_test/pytest_links/TestController.test_controller_construction_invalid \
                 geopmdpy_test/pytest_links/TestController.test_controller_run \
                 geopmdpy_test/pytest_links/TestController.test_controller_run_app_error \
                 geopmdpy_test/pytest_links/TestController.test_controller_run_app_rc \
                 geopmdpy_test/pytest_links/TestTimedLoop.test_timed_loop_fixed \
                 geopmdpy_test/pytest_links/TestTimedLoop.test_timed_loop_infinite \
                 geopmdpy_test/pytest_links/TestTimedLoop.test_timed_loop_invalid \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_creation_json \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_load_clients \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_add_remove_client \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_batch_server \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_batch_server_service_restart \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_batch_server_bad_service_restart \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_is_pid_valid \
                 geopmdpy_test/pytest_links/TestActiveSessions.test_watch_id \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_pre_exists \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_default_creation \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_with_perm \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_link_not_dir \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_file_not_dir \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_bad_perms \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_bad_user_owner \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_bad_group_owner \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_creation_nested_dirs \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_umask_restored_on_error \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_not_exists \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_is_directory \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_is_link \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_is_fifo \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_bad_permissions \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_bad_user_owner \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_file_bad_group_owner \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_read_valid_file \
                 geopmdpy_test/pytest_links/TestSecureFiles.test_secure_make_file \
                 geopmdpy_test/pytest_links/TestWriteLock.test_default_creation \
                 geopmdpy_test/pytest_links/TestWriteLock.test_nested_creation \
                 geopmdpy_test/pytest_links/TestWriteLock.test_creation_bad_path \
                 geopmdpy_test/pytest_links/TestWriteLock.test_creation_bad_file \
                 geopmdpy_test/pytest_links/TestMSRDataFiles.test_msr_data_files \
                 # end

TESTS += $(GEOPMDPY_TESTS)

geopmdpy-checkprogs: $(GEOPMDPY_TESTS)

PHONY_TARGETS += pytest-checkprogs

$(GEOPMDPY_TESTS): geopmdpy_test/pytest_links/%:
	mkdir -p geopmdpy_test/pytest_links
	rm -f $@
	ln -s $(abs_srcdir)/geopmdpy_test/geopmdpy_test.sh $@

CLEAN_LOCAL_TARGETS += clean-local-geopmdpy-script-links

clean-local-geopmdpy-script-links:
	rm -f geopmdpy_test/pytest_links/*
