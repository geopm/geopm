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
              docs/source/info.rst \
              docs/source/overview.rst \
              docs/source/publications.rst \
              docs/source/manpages.rst \
              docs/source/reference.rst \
              docs/source/requires.rst \
              docs/source/runtime.rst \
              docs/source/service_readme.rst \
              docs/source/service.rst \
              docs/source/signals_SKX.rst \
              docs/source/use_cases.rst \
              docs/source/geopm.7.rst \
              docs/source/geopmadmin.1.rst \
              docs/source/geopmagent.1.rst \
              docs/source/geopm_agent_c.3.rst \
              docs/source/geopm_agent_energy_efficient.7.rst \
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
              docs/source/GEOPM_CXX_MAN_EnergyEfficientAgent.3.rst \
              docs/source/GEOPM_CXX_MAN_EnergyEfficientRegion.3.rst \
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
              docs/source/geopmplotter.1.rst \
              docs/source/geopm_policystore_c.3.rst \
              docs/source/geopm_prof_c.3.rst \
              docs/source/geopmpy.7.rst \
              docs/source/geopmread.1.rst \
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
                docs/build/man/geopmread.1 \
                docs/build/man/geopm_report.7 \
                docs/build/man/geopm_sched.3 \
                docs/build/man/geopm_time.3 \
                docs/build/man/geopm_topo_c.3 \
                docs/build/man/geopm_version.3 \
                docs/build/man/geopmwrite.1 \
                # end

base_man = docs/build/man/geopmadmin.1 \
           docs/build/man/geopmagent.1 \
           docs/build/man/geopm_agent_c.3 \
           docs/build/man/geopm_agent_energy_efficient.7 \
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
           docs/build/man/GEOPM_CXX_MAN_EnergyEfficientAgent.3 \
           docs/build/man/GEOPM_CXX_MAN_EnergyEfficientRegion.3 \
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
           docs/build/man/geopmplotter.1 \
           docs/build/man/geopm_policystore_c.3 \
           docs/build/man/geopm_prof_c.3 \
           docs/build/man/geopmpy.7 \
           # end

all_man = $(dist_man_MANS) $(base_man)

$(all_man): docs/build/man/%: $(top_srcdir)/docs/source/%.rst
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M man $(abs_srcdir)/docs/source docs/build

docs: docs_man docs_html

docs_html: libgeopmd.la $(abs_srcdir)/geopmdpy/version.py
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M html $(abs_srcdir)/docs/source docs/build

docs_man: libgeopmd.la $(abs_srcdir)/geopmdpy/version.py
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M man $(abs_srcdir)/docs/source docs/build


clean-local-docs:
	rm -rf docs/build

CLEAN_LOCAL_TARGETS += clean-local-docs
PHONY_TARGETS += docs docs_html docs_man clean-local-docs base_man
