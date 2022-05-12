#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# The transform variable will transform any installed file beginning
# with "GEOPM_CXX_MAN_" to begin with "geopm::".  This is to avoid
# having make targets that contain the colon character.  See autotools
# documentation below.
#
# https://www.gnu.org/software/autoconf/manual/autoconf-2.67/html_node/Transformation-Rules.html
#
transform='s/GEOPM_CXX_MAN_/geopm::/'

EXTRA_DIST += docs/geninfo.sh \
              docs/source/admin.rst \
              docs/source/analysis.rst \
              docs/source/build.rst \
              docs/source/client.rst \
              docs/source/conf.py \
              docs/source/contrib.rst \
              docs/source/controls_SKX.rst \
              docs/source/devel.rst \
              docs/source/index.rst \
              docs/source/install.rst \
              docs/source/overview.rst \
              docs/source/publications.rst \
              docs/source/reference.rst \
              docs/source/requires.rst \
              docs/source/runtime.rst \
              docs/source/security.rst \
              docs/source/service_readme.rst \
              docs/source/service.rst \
              docs/source/signals_SKX.rst \
              docs/source/use_cases.rst \
              $(all_man_rst) \
              $(all_man_target) \
              # end

all_man_rst = docs/source/geopm.7.rst \
              docs/source/geopmaccess.1.rst \
              docs/source/geopmadmin.1.rst \
              docs/source/geopmagent.1.rst \
              docs/source/geopm_agent_c.3.rst \
              docs/source/geopm_agent_frequency_map.7.rst \
              docs/source/geopm_agent_monitor.7.rst \
              docs/source/geopm_agent_power_balancer.7.rst \
              docs/source/geopm_agent_power_governor.7.rst \
              docs/source/geopmbench.1.rst \
              docs/source/geopmctl.1.rst \
              docs/source/geopm_ctl_c.3.rst \
              docs/source/GEOPM_CXX_MAN_Agent.3.rst \
              docs/source/GEOPM_CXX_MAN_Agg.3.rst \
              docs/source/GEOPM_CXX_MAN_CircularBuffer.3.rst \
              docs/source/GEOPM_CXX_MAN_CNLIOGroup.3.rst \
              docs/source/GEOPM_CXX_MAN_Comm.3.rst \
              docs/source/GEOPM_CXX_MAN_CpuinfoIOGroup.3.rst \
              docs/source/GEOPM_CXX_MAN_Daemon.3.rst \
              docs/source/GEOPM_CXX_MAN_Endpoint.3.rst \
              docs/source/GEOPM_CXX_MAN_Exception.3.rst \
              docs/source/GEOPM_CXX_MAN_Helper.3.rst \
              docs/source/GEOPM_CXX_MAN_IOGroup.3.rst \
              docs/source/GEOPM_CXX_MAN_MonitorAgent.3.rst \
              docs/source/GEOPM_CXX_MAN_MPIComm.3.rst \
              docs/source/GEOPM_CXX_MAN_MSRIO.3.rst \
              docs/source/GEOPM_CXX_MAN_MSRIOGroup.3.rst \
              docs/source/GEOPM_CXX_MAN_PlatformIO.3.rst \
              docs/source/GEOPM_CXX_MAN_PlatformTopo.3.rst \
              docs/source/GEOPM_CXX_MAN_PluginFactory.3.rst \
              docs/source/GEOPM_CXX_MAN_PowerBalancer.3.rst \
              docs/source/GEOPM_CXX_MAN_PowerBalancerAgent.3.rst \
              docs/source/GEOPM_CXX_MAN_PowerGovernor.3.rst \
              docs/source/GEOPM_CXX_MAN_PowerGovernorAgent.3.rst \
              docs/source/GEOPM_CXX_MAN_ProfileIOGroup.3.rst \
              docs/source/GEOPM_CXX_MAN_SampleAggregator.3.rst \
              docs/source/GEOPM_CXX_MAN_SharedMemory.3.rst \
              docs/source/GEOPM_CXX_MAN_TimeIOGroup.3.rst \
              docs/source/geopm_daemon_c.3.rst \
              docs/source/geopmendpoint.1.rst \
              docs/source/geopm_endpoint_c.3.rst \
              docs/source/geopm_error.3.rst \
              docs/source/geopm_fortran.3.rst \
              docs/source/geopm_hash.3.rst \
              docs/source/geopm_imbalancer.3.rst \
              docs/source/geopmlaunch.1.rst \
              docs/source/geopm_pio_c.3.rst \
              docs/source/geopm_pio_sst.7.rst \
              docs/source/geopm_policystore_c.3.rst \
              docs/source/geopm_prof_c.3.rst \
              docs/source/geopmpy.7.rst \
              docs/source/geopmdpy.7.rst \
              docs/source/geopmread.1.rst \
              docs/source/geopmsession.1.rst \
              docs/source/geopm_report.7.rst \
              docs/source/geopm_sched.3.rst \
              docs/source/geopm_time.3.rst \
              docs/source/geopm_topo_c.3.rst \
              docs/source/geopm_version.3.rst \
              docs/source/geopmwrite.1.rst \
              # end

dist_man_MANS = docs/build/man/geopm.7 \
                docs/build/man/GEOPM_CXX_MAN_Agg.3 \
                docs/build/man/GEOPM_CXX_MAN_CircularBuffer.3 \
                docs/build/man/GEOPM_CXX_MAN_CNLIOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_CpuinfoIOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_Exception.3 \
                docs/build/man/GEOPM_CXX_MAN_Helper.3 \
                docs/build/man/GEOPM_CXX_MAN_IOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_MSRIO.3 \
                docs/build/man/GEOPM_CXX_MAN_MSRIOGroup.3 \
                docs/build/man/GEOPM_CXX_MAN_PlatformIO.3 \
                docs/build/man/GEOPM_CXX_MAN_PlatformTopo.3 \
                docs/build/man/GEOPM_CXX_MAN_PluginFactory.3 \
                docs/build/man/GEOPM_CXX_MAN_SampleAggregator.3 \
                docs/build/man/GEOPM_CXX_MAN_SharedMemory.3 \
                docs/build/man/GEOPM_CXX_MAN_TimeIOGroup.3 \
                docs/build/man/geopm_error.3 \
                docs/build/man/geopm_hash.3 \
                docs/build/man/geopm_pio_c.3 \
                docs/build/man/geopm_pio_sst.7 \
                docs/build/man/geopmread.1 \
                docs/build/man/geopm_report.7 \
                docs/build/man/geopm_sched.3 \
                docs/build/man/geopm_time.3 \
                docs/build/man/geopm_topo_c.3 \
                docs/build/man/geopm_version.3 \
                docs/build/man/geopmwrite.1 \
                docs/build/man/geopmaccess.1 \
                docs/build/man/geopmsession.1 \
                docs/build/man/geopmdpy.7 \
                # end

base_man = docs/build/man/geopmadmin.1 \
           docs/build/man/geopmagent.1 \
           docs/build/man/geopm_agent_c.3 \
           docs/build/man/geopm_agent_frequency_map.7 \
           docs/build/man/geopm_agent_monitor.7 \
           docs/build/man/geopm_agent_power_balancer.7 \
           docs/build/man/geopm_agent_power_governor.7 \
           docs/build/man/geopmbench.1 \
           docs/build/man/geopmctl.1 \
           docs/build/man/geopm_ctl_c.3 \
           docs/build/man/GEOPM_CXX_MAN_Agent.3 \
           docs/build/man/GEOPM_CXX_MAN_Comm.3 \
           docs/build/man/GEOPM_CXX_MAN_Daemon.3 \
           docs/build/man/GEOPM_CXX_MAN_Endpoint.3 \
           docs/build/man/GEOPM_CXX_MAN_MonitorAgent.3 \
           docs/build/man/GEOPM_CXX_MAN_MPIComm.3 \
           docs/build/man/GEOPM_CXX_MAN_PowerBalancer.3 \
           docs/build/man/GEOPM_CXX_MAN_PowerBalancerAgent.3 \
           docs/build/man/GEOPM_CXX_MAN_PowerGovernor.3 \
           docs/build/man/GEOPM_CXX_MAN_PowerGovernorAgent.3 \
           docs/build/man/GEOPM_CXX_MAN_ProfileIOGroup.3 \
           docs/build/man/geopm_daemon_c.3 \
           docs/build/man/geopmendpoint.1 \
           docs/build/man/geopm_endpoint_c.3 \
           docs/build/man/geopm_fortran.3 \
           docs/build/man/geopm_imbalancer.3 \
           docs/build/man/geopmlaunch.1 \
           docs/build/man/geopm_policystore_c.3 \
           docs/build/man/geopm_prof_c.3 \
           docs/build/man/geopmpy.7 \
           # end

all_man_target = docs/build/man/.dirfile
all_man = $(dist_man_MANS) $(base_man)
all_local += $(all_man_target)

docs: docs_man docs_html

if ENABLE_DOCS
$(all_man_target): $(all_man_rst)
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M man $(abs_srcdir)/docs/source docs/build
	touch $(all_man_target)

$(all_man): docs/build/man/%: $(top_srcdir)/docs/source/%.rst $(all_man_target)


docs_html: libgeopmd.la $(abs_srcdir)/geopmdpy/version.py
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M html $(abs_srcdir)/docs/source docs/build

docs_man: libgeopmd.la $(abs_srcdir)/geopmdpy/version.py
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M man $(abs_srcdir)/docs/source docs/build

else

$(all_man_target): $(all_man)
	touch $(all_man_target)

$(all_man): docs/build/man/%: $(top_srcdir)/docs/source/%.rst
	mkdir -p docs/build/man
	cp $^ $@

docs_html:

endif

clean-local-docs:
	rm -rf docs/build

CLEAN_LOCAL_TARGETS += clean-local-docs
PHONY_TARGETS += docs docs_html docs_man clean-local-docs base_man
