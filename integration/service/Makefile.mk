#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += service/README.md \
              service/build_dasbus.sh \
              service/check_session_clean.sh \
              service/get_batch_server.py \
              service/install_service.sh \
              service/kill_geopmd.sh \
              service/open_pbs/generate_coefficients_from_reports.py \
              service/open_pbs/geopm_install_pbs_power_limit.sh \
              service/open_pbs/geopm_openpbs_test.sh \
              service/open_pbs/geopm_pbs_hook_config.schema.json \
              service/open_pbs/geopm_power_limit.py \
              service/open_pbs/merge_config_files.py \
              service/open_pbs/README.rst \
              service/open_pbs/TestPowerLimitHook.py \
              service/test/__init__.py \
              service/test/test_bash_examples.py \
              service/test/check_write_session.sh \
              service/test/test_pio_reset.py \
              service/test/test_sst_priority.sh \
              service/test/test_su_give_access.sh \
              service/test/test_su_restart.sh \
              service/test/test_term_batch_client.sh \
              service/test/save_restore.sh \
              service/test/process_server_actions.sh \
              service/test/test_kill_batch_client.sh \
              service/test/test_kill_batch_server.sh \
              service/test/test_kill_geopmd_batch_run.sh \
              service/test/test_kill_geopmd_serial_run.sh \
              service/test/test_systemctl_stop_geopm.sh \
              service/test/batch_write_client_helper.py \
              service/test/batch_write_client_helper.sh \
              service/test/serial_write_client_helper.py \
              service/test/serial_write_client_helper.sh \
              service/test/do_write.sh \
              service/test/test_batch_perf.sh \
              service/test/plot_batch_perf.py \
              # end

noinst_PROGRAMS += service/test/test_batch_interface \
		   service/test/test_invalid_values \
		   service/test/test_batch_perf \
		   #end

# This test depends on non-installed libgeopmd headers
noinst_PROGRAMS += service/test/test_batch_server \
		    #end
service_test_test_batch_server_SOURCES = service/test/test_batch_server.cpp
service_test_test_batch_server_LDADD = libgeopmd.la

service_test_test_batch_interface_SOURCES = service/test/test_batch_interface.cpp
service_test_test_batch_interface_LDADD = libgeopmd.la

service_test_test_invalid_values_SOURCES = service/test/test_invalid_values.cpp
service_test_test_invalid_values_CXXFLAGS = $(CXXFLAGS) -fPIC -fPIE $(FASTMATH)
service_test_test_invalid_values_LDADD = libgeopmd.la

service_test_test_batch_perf_SOURCES = service/test/test_batch_perf.cpp
service_test_test_batch_perf_LDADD = libgeopmd.la
