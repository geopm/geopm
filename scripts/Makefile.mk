#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

prefix ?= /usr
exec_prefix ?= $(prefix)
bindir ?= $(prefix)/bin
libexecdir ?= $(exec_prefix)/libexec

EXTRA_DIST += scripts/MANIFEST.in \
              scripts/geopmanalysis \
              scripts/geopmconvertreport \
              scripts/geopmlaunch \
              scripts/geopmplotter \
              scripts/geopmpy/__init__.py \
              scripts/geopmpy/agent.py \
              scripts/geopmpy/analysis.py \
              scripts/geopmpy/error.py \
              scripts/geopmpy/io.py \
              scripts/geopmpy/launcher.py \
              scripts/geopmpy/pio.py \
              scripts/geopmpy/plotter.py \
              scripts/geopmpy/policy_store.py \
              scripts/geopmpy/topo.py \
              scripts/geopmpy/version.py \
              scripts/requirements.txt \
              scripts/setup.py \
              scripts/setup.py.in \
              scripts/test/TestAffinity.py \
              scripts/test/TestAgent.py \
              scripts/test/TestAnalysisCommandLine.py \
              scripts/test/TestBalancerAnalysis.py \
              scripts/test/TestError.py \
              scripts/test/TestFreqSweepAnalysis.py \
              scripts/test/TestIO.py \
              scripts/test/TestLauncher.py \
              scripts/test/TestNodeEfficiencyAnalysis.py \
              scripts/test/TestNodePowerAnalysis.py \
              scripts/test/TestPIO.py \
              scripts/test/TestPolicyStore.py \
              scripts/test/TestPolicyStoreIntegration.py \
              scripts/test/TestPowerSweepAnalysis.py \
              scripts/test/TestTopo.py \
              scripts/test/__init__.py \
              scripts/test/analysis_helper.py \
              scripts/test/mock_report.py \
              scripts/test/geopmpy_test.sh \
              scripts/test/check_python3_compatibility.sh \
              # end

PYTEST_TESTS = scripts/test/pytest_links/TestAffinity.test_affinity_0 \
               scripts/test/pytest_links/TestAffinity.test_affinity_1 \
               scripts/test/pytest_links/TestAffinity.test_affinity_10 \
               scripts/test/pytest_links/TestAffinity.test_affinity_11 \
               scripts/test/pytest_links/TestAffinity.test_affinity_12 \
               scripts/test/pytest_links/TestAffinity.test_affinity_4 \
               scripts/test/pytest_links/TestAffinity.test_affinity_5 \
               scripts/test/pytest_links/TestAffinity.test_affinity_6 \
               scripts/test/pytest_links/TestAffinity.test_affinity_7 \
               scripts/test/pytest_links/TestAffinity.test_affinity_8 \
               scripts/test/pytest_links/TestAffinity.test_affinity_9 \
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
               scripts/test/pytest_links/TestAnalysisCommandLine.test_bad_type \
               scripts/test/pytest_links/TestAnalysisCommandLine.test_help \
               scripts/test/pytest_links/TestAnalysisCommandLine.test_help_custom \
               scripts/test/pytest_links/TestAnalysisCommandLine.test_launch_only \
               scripts/test/pytest_links/TestAnalysisCommandLine.test_launch_plot_summary \
               scripts/test/pytest_links/TestAnalysisCommandLine.test_no_args \
               scripts/test/pytest_links/TestBalancerAnalysis.test_balancer_plot_process_energy \
               scripts/test/pytest_links/TestBalancerAnalysis.test_balancer_plot_process_power \
               scripts/test/pytest_links/TestBalancerAnalysis.test_balancer_plot_process_runtime \
               scripts/test/pytest_links/TestError.test_error_message \
               scripts/test/pytest_links/TestFreqSweepAnalysis.test_offline_baseline_comparison_report \
               scripts/test/pytest_links/TestFreqSweepAnalysis.test_online_baseline_comparison_report \
               scripts/test/pytest_links/TestFreqSweepAnalysis.test_region_freq_map \
               scripts/test/pytest_links/TestFreqSweepAnalysis.test_stream_dgemm_mix_report \
               scripts/test/pytest_links/TestIO.test_requested_online_frequency \
               scripts/test/pytest_links/TestIO.test_report \
               scripts/test/pytest_links/TestIO.test_report_cache \
               scripts/test/pytest_links/TestLauncher.test_process_count \
               scripts/test/pytest_links/TestLauncher.test_non_file_output \
               scripts/test/pytest_links/TestLauncher.test_main \
               scripts/test/pytest_links/TestNodeEfficiencyAnalysis.test_node_efficiency_process \
               scripts/test/pytest_links/TestNodePowerAnalysis.test_node_power_process \
               scripts/test/pytest_links/TestPowerSweepAnalysis.test_power_sweep_summary \
               scripts/test/pytest_links/TestPIO.test_domain_name \
               scripts/test/pytest_links/TestPIO.test_signal_names \
               scripts/test/pytest_links/TestPIO.test_control_names \
               scripts/test/pytest_links/TestPIO.test_read_signal \
               scripts/test/pytest_links/TestPIO.test_write_control \
               scripts/test/pytest_links/TestPolicyStore.test_connect \
               scripts/test/pytest_links/TestPolicyStore.test_disconnect \
               scripts/test/pytest_links/TestPolicyStore.test_get_best \
               scripts/test/pytest_links/TestPolicyStore.test_set_best \
               scripts/test/pytest_links/TestPolicyStore.test_set_default \
               scripts/test/pytest_links/TestPolicyStoreIntegration.test_all_interfaces \
               scripts/test/pytest_links/TestTopo.test_num_domain \
               scripts/test/pytest_links/TestTopo.test_domain_domain_nested \
               scripts/test/pytest_links/TestTopo.test_domain_name_type \
               # end

TESTS += scripts/test/check_python3_compatibility.sh \
	 $(PYTEST_TESTS)

pytest-checkprogs: $(PYTEST_TESTS)

PHONY_TARGETS += pytest-checkprogs

$(PYTEST_TESTS): scripts/test/pytest_links/%:
	mkdir -p scripts/test/pytest_links
	ln -s ../geopmpy_test.sh $@

clean-local-pytest-script-links:
	rm -f scripts/test/pytest_links/*

clean-local-python: scripts/setup.py
	cd scripts && ./setup.py clean --all

CLEAN_LOCAL_TARGETS += clean-local-pytest-script-links \
                       clean-local-python

install-python: scripts/setup.py
	cd scripts && ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)
