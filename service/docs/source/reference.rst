
Reference Manual
================


This reference manual contains the GEOPM manual pages and the signal and control descriptions.


GEOPM Manual Pages
------------------

.. toctree::
   :maxdepth: 2
   :caption: Overview

   geopm.7


.. toctree::
   :maxdepth: 1
   :caption: Command Line Interfaces

   geopmaccess.1
   geopmadmin.1
   geopmagent.1
   geopmbench.1
   geopmctl.1
   geopmendpoint.1
   geopmlaunch.1
   geopmplotter.1
   geopmread.1
   geopmsession.1
   geopmwrite.1


.. toctree::
   :maxdepth: 2
   :caption: GEOPM Python Interfaces

   geopmdpy.7
   geopmpy.7

* :ref:`genindex`


.. toctree::
   :maxdepth: 1
   :caption: GEOPM C Interfaces

   geopm_agent_c.3
   geopm_ctl_c.3
   geopm_daemon_c.3
   geopm_endpoint_c.3
   geopm_error.3
   geopm_fortran.3
   geopm_hash.3
   geopm_imbalancer.3
   geopm_pio_c.3
   geopm_policystore_c.3
   geopm_prof_c.3
   geopm_sched.3
   geopm_time.3
   geopm_topo_c.3
   geopm_version.3


.. toctree::
   :maxdepth: 1
   :caption: GEOPM C++ Interfaces

   GEOPM_CXX_MAN_Agent.3
   GEOPM_CXX_MAN_Agg.3
   GEOPM_CXX_MAN_CNLIOGroup.3
   GEOPM_CXX_MAN_CircularBuffer.3
   GEOPM_CXX_MAN_Comm.3
   GEOPM_CXX_MAN_CpuinfoIOGroup.3
   GEOPM_CXX_MAN_Daemon.3
   GEOPM_CXX_MAN_Endpoint.3
   GEOPM_CXX_MAN_EnergyEfficientAgent.3
   GEOPM_CXX_MAN_EnergyEfficientRegion.3
   GEOPM_CXX_MAN_Exception.3
   GEOPM_CXX_MAN_Helper.3
   GEOPM_CXX_MAN_IOGroup.3
   GEOPM_CXX_MAN_MPIComm.3
   GEOPM_CXX_MAN_MSRIO.3
   GEOPM_CXX_MAN_MSRIOGroup.3
   GEOPM_CXX_MAN_MonitorAgent.3
   GEOPM_CXX_MAN_PlatformIO.3
   GEOPM_CXX_MAN_PlatformTopo.3
   GEOPM_CXX_MAN_PluginFactory.3
   GEOPM_CXX_MAN_PowerBalancer.3
   GEOPM_CXX_MAN_PowerBalancerAgent.3
   GEOPM_CXX_MAN_PowerGovernor.3
   GEOPM_CXX_MAN_PowerGovernorAgent.3
   GEOPM_CXX_MAN_ProfileIOGroup.3
   GEOPM_CXX_MAN_SampleAggregator.3
   GEOPM_CXX_MAN_SharedMemory.3
   GEOPM_CXX_MAN_TimeIOGroup.3


.. toctree::
   :maxdepth: 1
   :caption: Output File Formats

   geopm_report.7


.. toctree::
   :maxdepth: 1
   :caption: GEOPM HPC Runtime Agents

   geopm_agent_energy_efficient.7
   geopm_agent_frequency_map.7
   geopm_agent_monitor.7
   geopm_agent_power_balancer.7
   geopm_agent_power_governor.7


Signals and Controls
--------------------

The signals and controls available to be read from the service may vary
depending on several factors including: the hardware installed on the system,
the device drivers that are enabled, the configuration of kernel modules and
other operating system settings.  The information about the signals and
controls available on one specific GEOPM enabled system is given below.

.. toctree::
   :maxdepth: 1
   :caption: Signals:

   signals_SKX

.. toctree::
   :maxdepth: 1
   :caption: Controls:

   controls_SKX
