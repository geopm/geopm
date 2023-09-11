#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

prefix ?= /usr
exec_prefix ?= $(prefix)
bindir ?= $(prefix)/bin
libexecdir ?= $(exec_prefix)/libexec

EXTRA_DIST += scripts/MANIFEST.in \
              scripts/geopmlaunch \
              scripts/geopmpy/__init__.py \
              scripts/geopmpy/agent.py \
              scripts/geopmpy/endpoint.py \
              scripts/geopmpy/io.py \
              scripts/geopmpy/hash.py \
              scripts/geopmpy/launcher.py \
              scripts/geopmpy/policy_store.py \
              scripts/geopmpy/version.py \
              scripts/requirements.txt \
              scripts/setup.py \
              scripts/test/TestAffinity.py \
              scripts/test/TestAgent.py \
              scripts/test/TestEndpoint.py \
              scripts/test/TestHash.py \
              scripts/test/TestIO.py \
              scripts/test/test_io_experiment.report \
              scripts/test/TestLauncher.py \
              scripts/test/TestPolicyStore.py \
              scripts/test/TestPolicyStoreIntegration.py \
              scripts/test/__init__.py \
              # end

PYTEST_TESTS = scripts/test/pytest_links/TestAffinity.test_affinity_0 \
               scripts/test/pytest_links/TestAffinity.test_affinity_1 \
               scripts/test/pytest_links/TestAffinity.test_affinity_10 \
               scripts/test/pytest_links/TestAffinity.test_affinity_11 \
               scripts/test/pytest_links/TestAffinity.test_affinity_12 \
               scripts/test/pytest_links/TestAffinity.test_affinity_13 \
               scripts/test/pytest_links/TestAffinity.test_affinity_14 \
               scripts/test/pytest_links/TestAffinity.test_affinity_15 \
               scripts/test/pytest_links/TestAffinity.test_affinity_16 \
               scripts/test/pytest_links/TestAffinity.test_affinity_17 \
               scripts/test/pytest_links/TestAffinity.test_affinity_18 \
               scripts/test/pytest_links/TestAffinity.test_affinity_4 \
               scripts/test/pytest_links/TestAffinity.test_affinity_5 \
               scripts/test/pytest_links/TestAffinity.test_affinity_6 \
               scripts/test/pytest_links/TestAffinity.test_affinity_7 \
               scripts/test/pytest_links/TestAffinity.test_affinity_8 \
               scripts/test/pytest_links/TestAffinity.test_affinity_9 \
               scripts/test/pytest_links/TestAffinity.test_affinity_tutorial_knl \
               scripts/test/pytest_links/TestAffinity.test_1rank_1thread \
               scripts/test/pytest_links/TestAffinity.test_1rank_geopm_os_app_shared \
               scripts/test/pytest_links/TestAffinity.test_1rank_geopm_os_app_shared_noht \
               scripts/test/pytest_links/TestAffinity.test_1rank_geopm_os_shared \
               scripts/test/pytest_links/TestAffinity.test_1rank_geopm_os_shared_noht \
               scripts/test/pytest_links/TestAffinity.test_1rank_leave_2_cores \
               scripts/test/pytest_links/TestAffinity.test_1rank_no_env_threads \
               scripts/test/pytest_links/TestAffinity.test_1rank_no_env_threads_noht \
               scripts/test/pytest_links/TestAffinity.test_2rank_no_env_threads \
               scripts/test/pytest_links/TestAffinity.test_2rank_no_env_threads_noht \
               scripts/test/pytest_links/TestAffinity.test_3rank_no_env_threads \
               scripts/test/pytest_links/TestAffinity.test_3rank_no_env_threads_noht \
               scripts/test/pytest_links/TestAffinity.test_per_core_rank_1reserved_noht \
               scripts/test/pytest_links/TestAffinity.test_per_core_rank_2reserved_noht \
               scripts/test/pytest_links/TestAffinity.test_per_core_rank_noht \
               scripts/test/pytest_links/TestAgent.test_agent_names \
               scripts/test/pytest_links/TestAgent.test_json \
               scripts/test/pytest_links/TestAgent.test_policy_names \
               scripts/test/pytest_links/TestAgent.test_sample_names \
               scripts/test/pytest_links/TestEndpoint.test_endpoint_creation_destruction \
               scripts/test/pytest_links/TestEndpoint.test_endpoint_entry_exit \
               scripts/test/pytest_links/TestEndpoint.test_endpoint_agent_name \
               scripts/test/pytest_links/TestEndpoint.test_wait_for_agent_attach \
               scripts/test/pytest_links/TestEndpoint.test_stop_wait_loop \
               scripts/test/pytest_links/TestEndpoint.test_reset_wait_loop \
               scripts/test/pytest_links/TestEndpoint.test_endpoint_profile_name \
               scripts/test/pytest_links/TestEndpoint.test_endpoint_nodes \
               scripts/test/pytest_links/TestEndpoint.test_write_policy \
               scripts/test/pytest_links/TestEndpoint.test_read_sample \
               scripts/test/pytest_links/TestHash.test_hash \
               scripts/test/pytest_links/TestIO.test_requested_online_frequency \
               scripts/test/pytest_links/TestIO.test_report_cache \
               scripts/test/pytest_links/TestIO.test_trace \
               scripts/test/pytest_links/TestIO.test_figure_of_merit \
               scripts/test/pytest_links/TestIO.test_h5_name \
               scripts/test/pytest_links/TestIO.test_raw_report_collection_region \
               scripts/test/pytest_links/TestIO.test_raw_report_collection_app_totals \
               scripts/test/pytest_links/TestIO.test_raw_report_collection_epoch_totals \
               scripts/test/pytest_links/TestIO.test_raw_report_collection_unmarked_totals \
               scripts/test/pytest_links/TestLauncher.test_affinity_disable \
               scripts/test/pytest_links/TestLauncher.test_affinity_enable \
               scripts/test/pytest_links/TestLauncher.test_main \
               scripts/test/pytest_links/TestLauncher.test_non_file_output \
               scripts/test/pytest_links/TestLauncher.test_process_count \
               scripts/test/pytest_links/TestLauncher.test_quoted_args \
               scripts/test/pytest_links/TestPolicyStore.test_connect \
               scripts/test/pytest_links/TestPolicyStore.test_disconnect \
               scripts/test/pytest_links/TestPolicyStore.test_get_best \
               scripts/test/pytest_links/TestPolicyStore.test_set_best \
               scripts/test/pytest_links/TestPolicyStore.test_set_default \
               # end

if ENABLE_BETA
    PYTEST_TESTS += scripts/test/pytest_links/TestPolicyStoreIntegration.test_all_interfaces
endif

TESTS += $(PYTEST_TESTS)

pytest-checkprogs: $(PYTEST_TESTS)

PHONY_TARGETS += pytest-checkprogs

$(PYTEST_TESTS): scripts/test/pytest_links/%:
	mkdir -p scripts/test/pytest_links
	rm -f $@
	ln -s $(abs_builddir)/scripts/test/geopmpy_test.sh $@

clean-local-pytest-script-links:
	rm -f scripts/test/pytest_links/*

clean-local-python: scripts/setup.py
	cd $(abs_srcdir)/scripts && $(PYTHON) ./setup.py clean --all

CLEAN_LOCAL_TARGETS += clean-local-pytest-script-links \
                       clean-local-python

install-python: scripts/setup.py
# Move version.py into source for out of place builds
	if [ ! -f $(abs_srcdir)/scripts/geopmpy/version.py ]; then \
	    cp scripts/geopmpy/version.py $(abs_srcdir)/scripts/geopmpy/version.py; \
	fi
	cd $(abs_srcdir)/scripts && $(PYTHON) ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)
