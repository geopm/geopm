.. role:: raw-html-m2r(raw)
   :format: html


geopm(7) -- global extensible open power manager
================================================






DESCRIPTION
-----------

The Global Extensible Open Power Manager (GEOPM) is a framework for
exploring power and energy optimizations on heterogeneous platforms.

JOB LAUNCH
----------

`geopmlaunch(1) <geopmlaunch.1.html>`_\ : Application launch wrapper

`geopmreport(7) <geopm_report.7.html>`_\ : Reports produced by launch

APPLICATION PROFILING
---------------------

`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ : Application profiling interfaces

`geopm_fortran(3) <geopm_fortran.3.html>`_\ : GEOPM Fortran interfaces

ANALYSIS TOOLS
--------------

`geopmread(1) <geopmread.1.html>`_\ : Query platform information

`geopmwrite(1) <geopmwrite.1.html>`_\ : Modify platform state

`geopm_topo_c(3) <geopm_topo_c.3.html>`_\ : Query platform component topology

`geopm_pio_c(3) <geopm_pio_c.3.html>`_\ : Interfaces to query and modify platform

`geopmbench(1) <geopmbench.1.html>`_\ : Synthetic benchmark application

BUILT-IN AGENTS
---------------

`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ : Agent implementation that enforces no policies

`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7.html>`_\ : Agent for running regions at user selected frequencies

`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ : Agent that optimizes performance under a power cap

`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ : Agent that enforces a power cap

RUNTIME CONTROL
---------------

`geopm_ctl_c(3) <geopm_ctl_c.3.html>`_\ : GEOPM runtime control thread

`geopmctl(1) <geopmctl.1.html>`_\ : GEOPM runtime control application

`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ : Query information about available agents

`geopmagent(1) <geopmagent.1.html>`_\ : Query agent information and create static policies

`geopmadmin(1) <geopmadmin.1.html>`_\ : Configure and check system wide GEOPM settings

MISC
----

`geopm_error(3) <geopm_error.3.html>`_\ : Error code descriptions

`geopm_version(3) <geopm_version.3.html>`_\ : GEOPM library version

`geopm_sched(3) <geopm_sched.3.html>`_\ : Interface with Linux scheduler

`geopm_time(3) <geopm_time.3.html>`_\ : Time related helper functions

`geopm_hash(3) <geopm_hash.3.html>`_\ : Numerical encoding helper functions

PLUGIN EXTENSION
----------------

`geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_\ : Plugin developer guide

`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_\ : High level platform abstraction

`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ : Plugin interface for platform

`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ : Plugin interface for monitor/control

INTEGRATION WITH PMPI
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

INTEGRATION WITH OMPT
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
``--geopm-ompt-disable`` option in `geopmlaunch(1) <geopmlaunch.1.html>`_

LAUNCHING THE RUNTIME
---------------------

The recommended method for launching the GEOPM runtime is the job
launch wrapper script `geopmlaunch(1) <geopmlaunch.1.html>`_.  See this man page for
details about the command line interface.  If `geopmlaunch(1) <geopmlaunch.1.html>`_ does
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
If using the "application" launch method then the `geopmctl(1) <geopmctl.1.html>`_
application should be launched in parallel.

There are legacy methods for launching the runtime programmatically.
These are documented in `geopm_ctl_c(3) <geopm_ctl_c.3.html>`_\ , but are deprecated as an
application-facing interface because their use within an application
is incompatible with the GEOPM launcher script.

CHOOSING AN AGENT AND POLICY
----------------------------

The Agent determines the optimization algorithm performed by the
runtime, and can be specified with the ``--geopm-agent`` option for the
launcher.  If not specified, by default the `geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_
is used to collect runtime statistics only, which will be summarized
in the report.

The constraints for the Agent algorithm are determined by the policy.
The policy can be provided as a file, through the ``--geopm-policy``
option for the launcher.  Policy files can be generated with the
`geopmagent(1) <geopmagent.1.html>`_ tool.  The values of the policy will be printed
in the header of the report.

If GEOPM has been configured with ``--enable-beta``\ , policies can also
be set through the endpoint, which should be manipulated by a system
administrator through an authority such as the resource manager.  Use
of the endpoint is described in `geopm_endpoint_c(3) <geopm_endpoint_c.3.html>`_.  In this
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

Refer to `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_ and the individual agent man pages for more
details on the behavior of the agents and their policies.  See
`geopmlaunch(1) <geopmlaunch.1.html>`_ for more details on the ``--geopm-agent``\ ,
``--geopm-policy``\ , ``--geopm-endpoint``\ , and
``--geopm-trace-endpoint-policy`` options.

INTERPRETING THE REPORT
-----------------------

If the ``GEOPM_REPORT`` environment variable is set then a report will
be generated.  There is one report file generated for each run.  The
format of the report, the data contained in it, and the controller's
sampling are described in `geopm_report(7) <geopm_report.7.html>`_.

INTERPRETING THE TRACE
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

ENVIRONMENT
-----------

When using the launcher wrapper script `geopmlaunch(1) <geopmlaunch.1.html>`_\ , the
interface to the GEOPM runtime is controlled by the launcher command
line options.  The launcher script sets the environment variables
described in this section according to the options specified on the
command line.  Direct use of these environment variables is only
recommended when launching the GEOPM runtime *without*
`geopmlaunch(1) <geopmlaunch.1.html>`_.  If launching the GEOPM controller in application
mode without `geopmlaunch(1) <geopmlaunch.1.html>`_\ , the environment variables documented
below must be set to the same values in the contexts where
`geopmctl(1) <geopmctl.1.html>`_ and the compute application are executed.

In addition to the environment, there are two node-local configuration
files that will impact the way the GEOPM behaves.  The location of
these files can be configured at compile time, but the default
locations are:

.. code-block::

   /etc/geopm/environment-default.json
   /etc/geopm/environment-override.json


The `geopmadmin(1) <geopmadmin.1.html>`_ tool can be used to display the location of
these files for your installation of GEOPM or to check the validity of
the system configuration.  These files contain JSON objects that map
GEOPM environment variables to default or override values.  The
``environment-default.json`` file will determine default values for the
GEOPM runtime in the case where the values are not set in the calling
environment.  The ``environment-override.json`` file will enforce that
any GEOPM process running on the compute node will use the values
specified regardless of the values set in the calling environment.

GEOPM ENVIRONMENT VARIABLES
---------------------------


* 
  ``GEOPM_REPORT``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-report``.

* 
  ``GEOPM_REPORT_SIGNALS``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-report-signals``.

* 
  ``GEOPM_TRACE``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-trace``.

* 
  ``GEOPM_TRACE_SIGNALS``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-trace-signals``.

* 
  ``GEOPM_TRACE_PROFILE``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-trace-profile``.

* 
  ``GEOPM_TRACE_ENDPOINT_POLICY``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-trace-endpoint-policy``.

* 
  ``GEOPM_PROFILE``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-profile``.

* 
  ``GEOPM_CTL``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-ctl``.

* 
  ``GEOPM_AGENT``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-agent``.

* 
  ``GEOPM_POLICY``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-policy``.

* 
  ``GEOPM_ENDPOINT``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-endpoint``.

* 
  ``GEOPM_SHMKEY``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-shmkey``.

* 
  ``GEOPM_TIMEOUT``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-timeout``.

* 
  ``GEOPM_PLUGIN_PATH``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-plugin-path``.

* 
  ``GEOPM_DEBUG_ATTACH``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-debug-attach``.

* 
  ``GEOPM_DISABLE_HYPERTHREADS``\ :
  See documentation for equivalent command line option to
  `geopmlaunch(1) <geopmlaunch.1.html>`_ called ``--geopm-hyperthreads-disable``.

* 
  ``GEOPM_OMPT_DISABLE``\ :
  Disable OpenMP region detection as described in `INTEGRATION WITH OMPT <INTEGRATION WITH OMPT_>`_.
  See documentation for equivalent command line option to `geopmlaunch(1) <geopmlaunch.1.html>`_
  called ``--geopm-ompt-disable``.

OTHER ENVIRONMENT VARIABLES
---------------------------


* 
  ``LD_DYNAMIC_WEAK``\ :
  The `geopmlaunch(1) <geopmlaunch.1.html>`_ tool will preload ``libgeopm.so`` for all
  applications, so the use of ``LD_DYNAMIC_WEAK`` is not required when
  using `geopmlaunch(1) <geopmlaunch.1.html>`_.  When not using `geopmlaunch(1) <geopmlaunch.1.html>`_
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

SEE ALSO
--------

`geopmpy(7) <geopmpy.7.html>`_\ ,
`geopmdpy(7) <geopmdpy.7.html>`_\ ,
`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7.html>`_\ ,
`geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_\ ,
`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7.html>`_\ ,
`geopm_agent_power_governor(7) <geopm_agent_power_governor.7.html>`_\ ,
`geopm_report(7) <geopm_report.7.html>`_\ ,
`geopm_agent_c(3) <geopm_agent_c.3.html>`_\ ,
`geopm_ctl_c(3) <geopm_ctl_c.3.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
`geopm_fortran(3) <geopm_fortran.3.html>`_\ ,
`geopm_hash(3) <geopm_hash.3.html>`_\ ,
`geopm_policystore_c(3) <geopm_policystore_c.3.html>`_\ ,
`geopm_pio_c(3) <geopm_pio_c.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopm_sched(3) <geopm_sched.3.html>`_\ ,
`geopm_time(3) <geopm_time.3.html>`_\ ,
`geopm_version(3) <geopm_version.3.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3.html>`_\ ,
`geopm::CircularBuffer(3) <GEOPM_CXX_MAN_CircularBuffer.3.html>`_\ ,
`geopm::CpuinfoIOGroup(3) <GEOPM_CXX_MAN_CpuinfoIOGroup.3.html>`_\ ,
`geopm::Exception(3) <GEOPM_CXX_MAN_Exception.3.html>`_\ ,
`geopm::Helper(3) <GEOPM_CXX_MAN_Helper.3.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
`geopm::MSRIO(3) <GEOPM_CXX_MAN_MSRIO.3.html>`_\ ,
`geopm::MSRIOGroup(3) <GEOPM_CXX_MAN_MSRIOGroup.3.html>`_\ ,
`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_\ ,
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_\ ,
`geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_\ ,
`geopm::PowerBalancer(3) <GEOPM_CXX_MAN_PowerBalancer.3.html>`_\ ,
`geopm::PowerGovernor(3) <GEOPM_CXX_MAN_PowerGovernor.3.html>`_\ ,
`geopm::ProfileIOGroup(3) <GEOPM_CXX_MAN_ProfileIOGroup.3.html>`_\ ,
`geopm::RegionAggregator(3) <GEOPM_CXX_MAN_RegionAggregator.3.html>`_\ ,
`geopm::SharedMemory(3) <GEOPM_CXX_MAN_SharedMemory.3.html>`_\ ,
`geopm::TimeIOGroup(3) <GEOPM_CXX_MAN_TimeIOGroup.3.html>`_\ ,
`geopmadmin(1) <geopmadmin.1.html>`_\ ,
`geopmagent(1) <geopmagent.1.html>`_\ ,
`geopmbench(1) <geopmbench.1.html>`_\ ,
`geopmctl(1) <geopmctl.1.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmaccess(1) <geopmaccess.1.html>`_\ ,
`geopmsession(1) <geopmsession.1.html>`_\ ,
`ld.so(8) <http://man7.org/linux/man-pages/man8/ld.so.8.html>`_
