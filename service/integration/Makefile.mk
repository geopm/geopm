#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/README.md \
              integration/build_dasbus.sh \
              integration/check_session_clean.sh \
              integration/get_batch_server.py \
              integration/install_service.sh \
              integration/kill_geopmd.sh \
              integration/open_pbs/geopm_install_pbs_power_limit.sh \
              integration/open_pbs/geopm_openpbs_test.sh \
              integration/open_pbs/geopm_power_limit.py \
              integration/open_pbs/README.rst \
              integration/open_pbs/TestPowerLimitHook.py \
              integration/test/__init__.py \
              integration/test/test_bash_examples.py \
              integration/test/check_write_session.sh \
              integration/test/test_batch_performance.py \
              integration/test/test_pio_reset.py \
              integration/test/test_sst_priority.sh \
              integration/test/test_su_give_access.sh \
              integration/test/test_su_restart.sh \
              integration/test/test_term_batch_client.sh \
              integration/test/save_restore.sh \
              integration/test/process_server_actions.sh \
              integration/test/test_kill_batch_client.sh \
              integration/test/test_kill_batch_server.sh \
              integration/test/test_kill_geopmd_batch_run.sh \
              integration/test/test_kill_geopmd_serial_run.sh \
              integration/test/test_systemctl_stop_geopm.sh \
              integration/test/batch_write_client_helper.py \
              integration/test/batch_write_client_helper.sh \
              integration/test/serial_write_client_helper.py \
              integration/test/serial_write_client_helper.sh \
              integration/test/do_write.sh \
              # end

if ENABLE_FUZZTESTS
check_PROGRAMS += integration/test/fuzztest_geopmhash \
                  #end
integration_test_fuzztest_geopmhash_SOURCES = integration/test/fuzztest_geopmhash_main.cpp
integration_test_fuzztest_geopmhash_CXXFLAGS = $(AM_CXXFLAGS) -fsanitize=fuzzer -fno-inline
integration_test_fuzztest_geopmhash_LDADD = libgeopmd.la
else
EXTRA_DIST += \
          integration/test/fuzztest_geopmhash_main.cpp \
          # end
endif

check_PROGRAMS += integration/test/test_batch_server \
                  integration/test/test_batch_interface \
                  integration/test/test_invalid_values \
                  #end

integration_test_test_batch_server_SOURCES = integration/test/test_batch_server.cpp
integration_test_test_batch_server_LDADD = libgeopmd.la

integration_test_test_batch_interface_SOURCES = integration/test/test_batch_interface.cpp
integration_test_test_batch_interface_LDADD = libgeopmd.la

integration_test_test_invalid_values_SOURCES = integration/test/test_invalid_values.cpp
integration_test_test_invalid_values_CXXFLAGS = $(CXXFLAGS) $(FASTMATH)
integration_test_test_invalid_values_LDADD = libgeopmd.la

TESTS += integration/open_pbs/geopm_openpbs_test.sh
