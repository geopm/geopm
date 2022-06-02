
geopm(7) -- global extensible open power manager
================================================


Description
-----------

The Global Extensible Open Power Manager (GEOPM) is a framework for
exploring power and energy optimizations on heterogeneous platforms.

Job Launch
----------

:doc:`geopmlaunch(1) <geopmlaunch.1>`\ : Application launch wrapper

:doc:`geopmreport(7) <geopm_report.7>`\ : Reports produced by launch

Application Profiling
---------------------

:doc:`geopm_prof_c(3) <geopm_prof_c.3>`\ : Application profiling interfaces

:doc:`geopm_fortran(3) <geopm_fortran.3>`\ : GEOPM Fortran interfaces

Analysis Tools
--------------

:doc:`geopmread(1) <geopmread.1>`\ : Query platform information

:doc:`geopmwrite(1) <geopmwrite.1>`\ : Modify platform state

:doc:`geopm_topo_c(3) <geopm_topo_c.3>`\ : Query platform component topology

:doc:`geopm_pio_c(3) <geopm_pio_c.3>`\ : Interfaces to query and modify platform

:doc:`geopmbench(1) <geopmbench.1>`\ : Synthetic benchmark application

Built-In Agents
---------------

:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`\ : Agent implementation that enforces no policies

:doc:`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7>`\ : Agent for running regions at user selected frequencies

:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`\ : Agent that optimizes performance under a power cap

:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`\ : Agent that enforces a power cap

Runtime Control
---------------

:doc:`geopm_ctl_c(3) <geopm_ctl_c.3>`\ : GEOPM runtime control thread

:doc:`geopmctl(1) <geopmctl.1>`\ : GEOPM runtime control application

:doc:`geopm_agent_c(3) <geopm_agent_c.3>`\ : Query information about available agents

:doc:`geopmagent(1) <geopmagent.1>`\ : Query agent information and create static policies

:doc:`geopmadmin(1) <geopmadmin.1>`\ : Configure and check system wide GEOPM settings

Misc
----

:doc:`geopm_error(3) <geopm_error.3>`\ : Error code descriptions

:doc:`geopm_version(3) <geopm_version.3>`\ : GEOPM library version

:doc:`geopm_sched(3) <geopm_sched.3>`\ : Interface with Linux scheduler

:doc:`geopm_time(3) <geopm_time.3>`\ : Time related helper functions

:doc:`geopm_hash(3) <geopm_hash.3>`\ : Numerical encoding helper functions

Plugin Extension
----------------

:doc:`geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3>`\ : Plugin developer guide

:doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>`\ : High level platform abstraction

:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ : Plugin interface for platform

:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ : Plugin interface for monitor/control

Integration With PMPI
---------------------

Linking to libgeopm will define symbols that intercept the MPI
interface through PMPI.  This can be disabled with the configure time
option ``--disable-mpi``\ , but is enabled by default.  See
``LD_DYNAMIC_WEAK`` environment variable description below for the
runtime requirements of the PMPI design.  When using the GEOPM PMPI
interposition other profilers which use the same method will be in
conflict.  The GEOPM runtime can create an application performance
profile report and a trace of the application runtime.  As such, GEOPM
serves the role of an application profiler in addition to management
of power resources.  The report and trace generation are controlled by
the environment variables ``GEOPM_REPORT`` and ``GEOPM_TRACE``\ ; see
description below.

Integration With OMPT
---------------------

Unless the GEOPM runtime is configured to disable OpenMP, the library
is compiled against the OpenMP runtime.  If the OpenMP implementation
that GEOPM is compiled against supports the OMPT callbacks, then GEOPM
will use the OMPT callbacks to wrap OpenMP parallel regions with calls
to ``geopm_prof_enter()`` and ``geopm_prof_exit()``.  In this way, any
OpenMP parallel region not within another application-defined region
will be reported to the GEOPM runtime.  This will appear in the report
as a region name beginning with ``"[OMPT]"`` and referencing the object
file and function name containing the OpenMP parallel region e.g.

``[OMPT]geopmbench:geopm::StreamModelRegion::run()``

To expressly enable this feature, pass the ``--enable-ompt`` configure
flag at GEOPM configure time.  This will build and install the LLVM OpenMP
runtime configured to support OMPT if the default OpenMP runtime does
not support the OMPT callbacks.  Note that your compiler must be
compatible with the LLVM OpenMP ABI for extending it in this way.

This feature can be disabled on a per-run basis by setting the
``GEOPM_OMPT_DISABLE`` environment variable, or by using the
``--geopm-ompt-disable`` option in :doc:`geopmlaunch(1) <geopmlaunch.1>`

Launching The Runtime
---------------------

The recommended method for launching the GEOPM runtime is the job
launch wrapper script :doc:`geopmlaunch(1) <geopmlaunch.1>`.  See this man page for
details about the command line interface.  If :doc:`geopmlaunch(1) <geopmlaunch.1>` does
not provide an application launcher supported by your system, please
make a change request for support of the job launch method used on
your system at the github issues page:

https://github.com/geopm/geopm/issues

Also, consider porting your job launch command into the
``geopmpy.launcher`` module and submitting a change request as described
in ``CONTRIBUTING.md``.

If the job launch application is not supported by the ``geopmpy.launcher``
the recommended method is to use the environment variables described
in this man page including the ``GEOPM_CTL`` environment variable.
If using the "application" launch method then the :doc:`geopmctl(1) <geopmctl.1>`
application should be launched in parallel.

There are legacy methods for launching the runtime programmatically.
These are documented in :doc:`geopm_ctl_c(3) <geopm_ctl_c.3>`\ , but are deprecated as an
application-facing interface because their use within an application
is incompatible with the GEOPM launcher script.

Choosing An Agent And Policy
----------------------------

The Agent determines the optimization algorithm performed by the
runtime, and can be specified with the ``--geopm-agent`` option for the
launcher.  If not specified, by default the :doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`
is used to collect runtime statistics only, which will be summarized
in the report.

The constraints for the Agent algorithm are determined by the policy.
The policy can be provided as a file, through the ``--geopm-policy``
option for the launcher.  Policy files can be generated with the
:doc:`geopmagent(1) <geopmagent.1>` tool.  The values of the policy will be printed
in the header of the report.

If GEOPM has been configured with ``--enable-beta``\ , policies can also
be set through the endpoint, which should be manipulated by a system
administrator through an authority such as the resource manager.  Use
of the endpoint is described in :doc:`geopm_endpoint_c(3) <geopm_endpoint_c.3>`.  In this
scenario, users launching GEOPM may not be required or allowed to
specify the Agent or policy, if it has been set through the default
environment as described in the ``ENVIRONMENT`` section below.  If not
specified in the default environment, the location of the endpoint
should be provided through ``--geopm-endpoint``\ ; this option supercedes
the use of ``--geopm-policy``.  When GEOPM receives the policy through
the endpoint, the report will contain ``"DYNAMIC"`` for the value of the
policy.  The specific values received over time can be viewed through
use of the optional trace file enabled by
``--geopm-trace-endpoint-policy``.

Refer to :doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>` and the individual agent man pages for more
details on the behavior of the agents and their policies.  See
:doc:`geopmlaunch(1) <geopmlaunch.1>` for more details on the ``--geopm-agent``\ ,
``--geopm-policy``\ , ``--geopm-endpoint``\ , and
``--geopm-trace-endpoint-policy`` options.

Interpreting The Report
-----------------------

If the ``GEOPM_REPORT`` environment variable is set then a report will
be generated.  There is one report file generated for each run.  The
format of the report, the data contained in it, and the controller's
sampling are described in :doc:`geopm_report(7) <geopm_report.7>`.

Interpreting The Trace
----------------------

If the ``GEOPM_TRACE`` environment variable is set (see below) then a
trace file with time ordered information about the application runtime
is generated.  A separate trace file is generated for each compute
node and each file is a pipe (the ``|`` character) delimited **ASCII**
table. The file begins with a header that is marked by lines that
start with the ``#`` character.  The header contains information about
the GEOPM version, job start time, profile name (job description), and
agent that were used during the run.

The first row following the header gives a description of each field.
A simple method for selecting fields from the trace file is with the
``awk`` command:

.. code-block:: bash

   $ grep -v '^#' geopm.trace-host0 | awk -F\| '{print $1, $2, $11}'


will print a subset of the fields in the trace file called
``"geopm.trace-host0"``.

Environment
-----------

When using the launcher wrapper script :doc:`geopmlaunch(1) <geopmlaunch.1>`\ , the
interface to the GEOPM runtime is controlled by the launcher command
line options.  The launcher script sets the environment variables
described in this section according to the options specified on the
command line.  Direct use of these environment variables is only
recommended when launching the GEOPM runtime *without*
:doc:`geopmlaunch(1) <geopmlaunch.1>`.  If launching the GEOPM controller in application
mode without :doc:`geopmlaunch(1) <geopmlaunch.1>`\ , the environment variables documented
below must be set to the same values in the contexts where
:doc:`geopmctl(1) <geopmctl.1>` and the compute application are executed.

In addition to the environment, there are two node-local configuration
files that will impact the way the GEOPM behaves.  The location of
these files can be configured at compile time, but the default
locations are:

.. code-block::

   /etc/geopm/environment-default.json
   /etc/geopm/environment-override.json


The :doc:`geopmadmin(1) <geopmadmin.1>` tool can be used to display the location of
these files for your installation of GEOPM or to check the validity of
the system configuration.  These files contain JSON objects that map
GEOPM environment variables to default or override values.  The
``environment-default.json`` file will determine default values for the
GEOPM runtime in the case where the values are not set in the calling
environment.  The ``environment-override.json`` file will enforce that
any GEOPM process running on the compute node will use the values
specified regardless of the values set in the calling environment.

GEOPM Environment Variables
---------------------------


*
  ``GEOPM_REPORT``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-report``.

*
  ``GEOPM_REPORT_SIGNALS``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-report-signals``.

*
  ``GEOPM_TRACE``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-trace``.

*
  ``GEOPM_TRACE_SIGNALS``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-trace-signals``.

*
  ``GEOPM_TRACE_PROFILE``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-trace-profile``.

*
  ``GEOPM_TRACE_ENDPOINT_POLICY``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-trace-endpoint-policy``.

*
  ``GEOPM_PROFILE``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-profile``.

*
  ``GEOPM_CTL``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-ctl``.

*
  ``GEOPM_AGENT``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-agent``.

*
  ``GEOPM_POLICY``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-policy``.

*
  ``GEOPM_ENDPOINT``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-endpoint``.

*
  ``GEOPM_SHMKEY``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-shmkey``.

*
  ``GEOPM_TIMEOUT``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-timeout``.

*
  ``GEOPM_PLUGIN_PATH``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-plugin-path``.

*
  ``GEOPM_DEBUG_ATTACH``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-debug-attach``.

*
  ``GEOPM_DISABLE_HYPERTHREADS``\ :
  See documentation for equivalent command line option to
  :doc:`geopmlaunch(1) <geopmlaunch.1>` called ``--geopm-hyperthreads-disable``.

*
  ``GEOPM_OMPT_DISABLE``\ :
  Disable OpenMP region detection as described in `INTEGRATION WITH OMPT <INTEGRATION WITH OMPT_>`_.
  See documentation for equivalent command line option to :doc:`geopmlaunch(1) <geopmlaunch.1>`
  called ``--geopm-ompt-disable``.

Other Environment Variables
---------------------------


*
  ``LD_DYNAMIC_WEAK``\ :
  The :doc:`geopmlaunch(1) <geopmlaunch.1>` tool will preload ``libgeopm.so`` for all
  applications, so the use of ``LD_DYNAMIC_WEAK`` is not required when
  using :doc:`geopmlaunch(1) <geopmlaunch.1>`.  When not using :doc:`geopmlaunch(1) <geopmlaunch.1>`
  setting ``LD_DYNAMIC_WEAK`` may be required, see next paragraph for
  details.

  When dynamically linking an application to ``libgeopm`` for any
  features supported by the PMPI profiling of the MPI runtime it may
  be required that the ``LD_DYNAMIC_WEAK`` environment variable be set
  at runtime as is documented in the `ld.so(8) <http://man7.org/linux/man-pages/man8/ld.so.8.html>`_ man page.  When
  dynamically linking an application, if care is taken to link the
  ``libgeopm`` library before linking the library providing the weak MPI
  symbols, e.g. ``"-lgeopm -lmpi"``, linking order precedence will
  enforce the required override of the MPI interface symbols and the
  ``LD_DYNAMIC_WEAK`` environment variable is not required at runtime.

See Also
--------

:doc:`geopmpy(7) <geopmpy.7>`\ ,
:doc:`geopmdpy(7) <geopmdpy.7>`\ ,
:doc:`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7>`\ ,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`\ ,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`\ ,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`\ ,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopm_pio_cnl(7) <geopm_pio_cnl.7>`,
:doc:`geopm_pio_cpuinfo(7) <geopm_pio_cpuinfo.7>`\ ,
:doc:`geopm_pio_dcgm(7) <geopm_pio_dcgm.7>`\ ,
:doc:`geopm_pio_levelzero(7) <geopm_pio_levelzero.7>`\ ,
:doc:`geopm_pio_msr(7) <geopm_pio_msr.7>`\ ,
:doc:`geopm_pio_nvml(7) <geopm_pio_nvml.7>`\ ,
:doc:`geopm_pio_sst(7) <geopm_pio_sst.7>`\ ,
:doc:`geopm_pio_time(7) <geopm_pio_time.7>`\ ,
:doc:`geopm_report(7) <geopm_report.7>`\ ,
:doc:`geopm_agent_c(3) <geopm_agent_c.3>`\ ,
:doc:`geopm_ctl_c(3) <geopm_ctl_c.3>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`\ ,
:doc:`geopm_fortran(3) <geopm_fortran.3>`\ ,
:doc:`geopm_hash(3) <geopm_hash.3>`\ ,
:doc:`geopm_policystore_c(3) <geopm_policystore_c.3>`\ ,
:doc:`geopm_pio_c(3) <geopm_pio_c.3>`\ ,
:doc:`geopm_prof_c(3) <geopm_prof_c.3>`\ ,
:doc:`geopm_sched(3) <geopm_sched.3>`\ ,
:doc:`geopm_time(3) <geopm_time.3>`\ ,
:doc:`geopm_version(3) <geopm_version.3>`\ ,
:doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`\ ,
:doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`\ ,
:doc:`geopm::CircularBuffer(3) <GEOPM_CXX_MAN_CircularBuffer.3>`\ ,
:doc:`geopm::CpuinfoIOGroup(3) <GEOPM_CXX_MAN_CpuinfoIOGroup.3>`\ ,
:doc:`geopm::Exception(3) <GEOPM_CXX_MAN_Exception.3>`\ ,
:doc:`geopm::Helper(3) <GEOPM_CXX_MAN_Helper.3>`\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopm::MSRIO(3) <GEOPM_CXX_MAN_MSRIO.3>`\ ,
:doc:`geopm::MSRIOGroup(3) <GEOPM_CXX_MAN_MSRIOGroup.3>`\ ,
:doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>`\ ,
:doc:`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3>`\ ,
:doc:`geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3>`\ ,
:doc:`geopm::PowerBalancer(3) <GEOPM_CXX_MAN_PowerBalancer.3>`\ ,
:doc:`geopm::PowerGovernor(3) <GEOPM_CXX_MAN_PowerGovernor.3>`\ ,
:doc:`geopm::ProfileIOGroup(3) <GEOPM_CXX_MAN_ProfileIOGroup.3>`\ ,
:doc:`geopm::RegionAggregator(3) <GEOPM_CXX_MAN_RegionAggregator.3>`\ ,
:doc:`geopm::SharedMemory(3) <GEOPM_CXX_MAN_SharedMemory.3>`\ ,
:doc:`geopm::TimeIOGroup(3) <GEOPM_CXX_MAN_TimeIOGroup.3>`\ ,
:doc:`geopmadmin(1) <geopmadmin.1>`\ ,
:doc:`geopmagent(1) <geopmagent.1>`\ ,
:doc:`geopmbench(1) <geopmbench.1>`\ ,
:doc:`geopmctl(1) <geopmctl.1>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmaccess(1) <geopmaccess.1>`\ ,
:doc:`geopmsession(1) <geopmsession.1>`\ ,
`ld.so(8) <http://man7.org/linux/man-pages/man8/ld.so.8.html>`_
