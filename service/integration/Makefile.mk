#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += integration/README.md \
              integration/build_dasbus.sh \
              integration/install_service.sh \
              integration/test/__init__.py \
              integration/test/test_bash_examples.py \
              integration/test/check_write_session.sh \
              integration/test/test_batch_performance.py \
              integration/test/test_pio_reset.py \
              integration/test/test_sst_priority.sh \
              integration/test/test_su_give_access.sh \
              integration/test/test_su_restart.sh \
              integration/test/test_su_term_batch.sh \
              integration/test/test_su_term_batch_helper.py \
              integration/test/test_su_term_batch_helper.sh \
              integration/test/do_write.sh \
              # end

check_PROGRAMS += integration/test/test_batch_server \
                  integration/test/test_batch_interface \
                  #end

integration_test_test_batch_server_SOURCES = integration/test/test_batch_server.cpp
integration_test_test_batch_server_LDADD = libgeopmd.la

integration_test_test_batch_interface_SOURCES = integration/test/test_batch_interface.cpp
integration_test_test_batch_interface_LDADD = libgeopmd.la
