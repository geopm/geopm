#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += README.md \
              build_dasbus.sh \
              check_session_clean.sh \
              get_batch_server.py \
              install_service.sh \
              kill_geopmd.sh \
              open_pbs/generate_coefficients_from_reports.py \
              open_pbs/geopm_install_pbs_power_limit.sh \
              open_pbs/geopm_openpbs_test.sh \
              open_pbs/geopm_pbs_hook_config.schema.json \
              open_pbs/geopm_power_limit.py \
              open_pbs/merge_config_files.py \
              open_pbs/README.rst \
              open_pbs/TestPowerLimitHook.py \
              test/__init__.py \
              test/test_bash_examples.py \
              test/check_write_session.sh \
              test/test_pio_reset.py \
              test/test_sst_priority.sh \
              test/test_su_give_access.sh \
              test/test_su_restart.sh \
              test/test_term_batch_client.sh \
              test/save_restore.sh \
              test/process_server_actions.sh \
              test/test_kill_batch_client.sh \
              test/test_kill_batch_server.sh \
              test/test_kill_geopmd_batch_run.sh \
              test/test_kill_geopmd_serial_run.sh \
              test/test_systemctl_stop_geopm.sh \
              test/batch_write_client_helper.py \
              test/batch_write_client_helper.sh \
              test/serial_write_client_helper.py \
              test/serial_write_client_helper.sh \
              test/do_write.sh \
              test/test_batch_perf.sh \
              test/plot_batch_perf.py \
              # end

noinst_PROGRAMS += test/test_batch_server \
                  test/test_batch_interface \
                  test/test_invalid_values \
                  test/test_batch_perf \
                  #end

test_test_batch_server_SOURCES = test/test_batch_server.cpp
test_test_batch_server_LDADD = libgeopmd.la

test_test_batch_interface_SOURCES = test/test_batch_interface.cpp
test_test_batch_interface_LDADD = libgeopmd.la

test_test_invalid_values_SOURCES = test/test_invalid_values.cpp
test_test_invalid_values_CXXFLAGS = $(CXXFLAGS) -fPIC -fPIE $(FASTMATH)
test_test_invalid_values_LDADD = libgeopmd.la

test_test_batch_perf_SOURCES = test/test_batch_perf.cpp
test_test_batch_perf_LDADD = libgeopmd.la

TESTS += open_pbs/geopm_openpbs_test.sh
