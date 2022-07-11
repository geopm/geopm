geopm(7) -- Global Extensible Open Power Manager
================================================

Description
-----------
The Global Extensible Open Power Manager (GEOPM) is a framework for
exploring power and energy optimizations on heterogeneous platforms. This
manual page outlines key tools and interfaces to configure and use GEOPM
software. The rest of this description outlines the features that are described
by sections of this manual page.

The GEOPM HPC runtime monitors platform-level metrics (e.g., CPU power, average
core frequency, elapsed time) while an application executes on the platform.
GEOPM provides :ref:`geopm.7:job launch` tools to execute an application with
the GEOPM HPC runtime and to read the platform-level metrics. GEOPM can also
report metrics with respect to application-specific regions or checkpoints by
utilizing optional :ref:`geopm.7:application profiling` interfaces. Several
:ref:`geopm.7:analysis tools` provide the means to directly query and modify
platform state.

The GEOPM HPC runtime interacts with a C++ framework to enable
custom power management algorithms (called *agents*) that utilize the analysis
tools. Some :ref:`geopm.7:built-In agents` are available by default. Additional
agents and platform input/output interfaces (called *IOGroups*) can be added to
the GEOPM HPC runtime by creating a :ref:`geopm.7:plugin extension`.

Although GEOPM supports manual instrumentation through
:ref:`geopm.7:application profiling` interfaces. Some automatic instrumentation
is also supported. :ref:`geopm.7:integration with PMPI` enables GEOPM to
automatically account for time spent inside MPI function calls by default.
:ref:`geopm.7:integration with OMPT` enables GEOPM to account for time spent in
individual OpenMP parallel regions.


Job Launch
----------
The :doc:`geopmlaunch(1) <geopmlaunch.1>` script is the recommended method for
launching the GEOPM HPC runtime. Unless modified by command-line arguments or
environment variables, GEOPM will create a ``geopm.report`` file with a summary
of metrics from application execution. See :doc:`geopmreport(7)
<geopm_report.7>`: for documentation about the output file format.

If :doc:`geopmlaunch(1) <geopmlaunch.1>` does not provide an application
launcher supported by your system, please make a change request to support the
job launch method used on your system at the github issues page:

https://github.com/geopm/geopm/issues

Also, consider porting your job launch command into the
:py:mod:`geopmpy.launcher` module and submitting a change request as described
in :doc:`contrib`.

If the job launch application is not supported by the :py:mod:`geopmpy.launcher`
the recommended method is to use the environment variables described
in this man page including the ``GEOPM_CTL`` environment variable.
If you use the *application* launch method, then you also need to launch the
:doc:`geopmctl(1) <geopmctl.1>` application in parallel to the application you
wish to run with GEOPM.

There are legacy methods for launching the runtime programmatically.
These are documented in :doc:`geopm_ctl(3) <geopm_ctl.3>`, but are deprecated as an
application-facing interface because their use within an application
is incompatible with the GEOPM launcher script.


Application Profiling
---------------------
GEOPM's provides application profiling interfaces for the C and Fortran
programming languages, documented in :doc:`geopm_prof(3) <geopm_prof.3>`
and :doc:`geopm_fortran(3) <geopm_fortran.3>`, respectively. These interfaces
enable an application to inform GEOPM of key monitoring events, such as entry
or exit from regions of interest, entry to a new iteration of a key looping
construct, and hints about the nature of the active region of code (e.g.,
whether the code is expected to be compute-bound, network-bound, or something
else).

The GEOPM HPC runtime profiles applications while executing as a separate
process or thread within the launched application, or as a separate
:doc:`geopmctl(1) <geopmctl.1>` application. C interfaces to drive the GEOPM
controller are documented in :doc:`geopm_ctl(3) <geopm_ctl.3>`.

Analysis Tools
--------------
GEOPM may also be used as a tooling interface for system analysis.

The :doc:`geopmread(1) <geopmread.1>` application reports the current values of
platform *signals* at varying levels of scope (*domains*). The
:doc:`geopmwrite(1) <geopmwrite.1>` application enables modulation of platform
*controls* at varying domains. Information about signals and controls
is documented at :doc:`geopm_pio(7) <geopm_pio.7>`. Programmatic interfaces for
read and write operations are available through :doc:`geopm_pio(3)
<geopm_pio.3>`.

The types of domains and their relationships with each other can be
programmatically queried through :doc:`geopm_topo(3) <geopm_topo.3>`.

GEOPM comes bundled with a synthetic benchmark application :doc:`geopmbench(1)
<geopmbench.1>`, which can be used as an application workload for basic analysis
and to experiment with the impact that signals and controls have on applications
under GEOPM.

Built-In Agents
---------------
GEOPM comes packaged with several built-in power management algorithms (*agents*):

* :doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`: An agent implementation that enforces no policies.
* :doc:`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7>`: An agent that applies user-selected frequencies at specific regions in the launched application.
* :doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`: An agent that optimizes performance under a power cap across multiple CPU packages.
* :doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`: An agent that enforces a power cap.
* :doc:`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7>` : An agent that sets GPU frequency based on GPU compute activity
* :doc:`geopm_agent_frequency_balancer(7) <geopm_agent_frequency_balancer.7>`: An agent that reduces imbalance through CPU core frequency controls.

Use the :doc:`geopmagent(1) <geopmagent.1>` application or the
:doc:`geopm_agent(3) <geopm_agent.3>` C interface to query agent
information and create static policies.

Plugin Extension
----------------
If you wish to monitor or control platform interfaces (*IOGroups*) that are not part of the
core GEOPM distribution, or if you wish to execute GEOPM agents that are not
part of the core distribution, then you can extend GEOPM with additional
IOGroup and agent plugins.

Agents and IOGroups are defined as C++ classes, documented in
:doc:`geopm::Agent(3) <geopm::Agent.3>` and :doc:`geopm::IOGroup(3)
<geopm::IOGroup.3>`, respectively. Both can be registered with GEOPM
through the :doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>`
interface. The :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`
interface provides a channel through which agents and GEOPM tools can interact
with IOGroups.

Integration With PMPI
---------------------
Linking to ``libgeopm`` will define symbols that intercept calls to the MPI
interface through PMPI.  This can be disabled with the configure time
option ``--disable-mpi``, but is enabled by default.  See the
``LD_DYNAMIC_WEAK`` environment variable description below for the
runtime requirements of the PMPI design.  When using the GEOPM PMPI
interposition other profilers which use the same method will be in
conflict.  The GEOPM runtime can create an application performance
profile report and a trace of the application runtime.  As such, GEOPM
serves the role of an application profiler in addition to management
of power resources.  The report and trace generation are controlled by
the environment variables ``GEOPM_REPORT`` and ``GEOPM_TRACE``; see
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

This feature can be enabled on a per-run basis by setting the
``GEOPM_OMPT_ENABLE`` environment variable, or by using the
``--geopm-ompt-enable`` option in :doc:`geopmlaunch(1) <geopmlaunch.1>`

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
of the endpoint is described in :doc:`geopm_endpoint(3) <geopm_endpoint.3>`.  In this
scenario, users launching GEOPM may not be required or allowed to
specify the Agent or policy, if it has been set through the default
environment as described in the ``ENVIRONMENT`` section below.  If not
specified in the default environment, the location of the endpoint
should be provided through ``--geopm-endpoint``\ ; this option supersedes
the use of ``--geopm-policy``.  When GEOPM receives the policy through
the endpoint, the report will contain ``"DYNAMIC"`` for the value of the
policy.  The specific values received over time can be viewed through
use of the optional trace file enabled by
``--geopm-trace-endpoint-policy``.

Refer to :doc:`geopm::Agent(3) <geopm::Agent.3>` and the individual agent man pages for more
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
``GEOPM_NUM_PROC``
  The number of processes to be tracked and profiled by the controller on each
  compute node. The controller will wait until this number of processes request
  profiling before starting the control loop and subsequent requests for
  profiling will be ignored by the controller.  If not set, the controller will
  wait until at least one process has registered for profiling before beginning
  the control loop.  The default value for GEOPM_NUM_PROC is one except when
  using the ``geopmlaunch`` CLI.  The ``geopmlaunch`` tool will infer this
  parameter based on the values passed to the underlying launch command, so the
  user does not have to set it explicitly.
``GEOPM_PROGRAM_FILTER``
  Required comma separated list of program invocation names of
  processes which are intended to be profiled and tracked by the
  controller.  See the ``--geopm-program-filter``
  :ref:`option description <geopm-program-filter option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for details.
``GEOPM_REPORT``
  The path to which a GEOPM report file is saved. See the
  ``--geopm-report`` :ref:`option description <geopm-report option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_REPORT_SIGNALS``
  Additional signals that are included in a GEOPM report. See the
  ``--geopm-report-signals`` :ref:`option description <geopm-report-signals
  option>` in :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_TRACE``
  The path and base name to which each per-host GEOPM trace file is saved. See the
  ``--geopm-trace`` :ref:`option description <geopm-trace option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_TRACE_SIGNALS``
  Additional signals that are included in a GEOPM trace. See the
  ``--geopm-trace-signals`` :ref:`option description <geopm-trace-signals
  option>` in :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_TRACE_PROFILE``
  The path and base name to which each per-host GEOPM profile trace file is
  saved. See the ``--geopm-trace-profile`` :ref:`option description
  <geopm-trace-profile option>` in :doc:`geopmlaunch(1) <geopmlaunch.1>` for
  more details.
``GEOPM_TRACE_ENDPOINT_POLICY``
  The path to an endpoint policy trace file is generated. See the
  ``--geopm-trace-endpoint-policy`` :ref:`option description <geopm-trace-endpoint-policy
  option>` in :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_PROFILE``
  The name of the profile written in the GEOPM report file. See the
  ``--geopm-profile`` :ref:`option description <geopm-profile option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_CTL``
  The type of GEOPM controller to use. See the
  ``--geopm-ctl`` :ref:`option description <geopm-ctl option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_AGENT``
  The type of agent to run in the GEOPM HPC runtime. See the
  ``--geopm-agent`` :ref:`option description <geopm-agent option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_POLICY``
  The path to the GEOPM policy JSON file to use for the selected agent. See the
  ``--geopm-policy`` :ref:`option description <geopm-policy option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_ENDPOINT``
  The prefix for shared memory keys used by the GEOPM endpoint. See the
  ``--geopm-endpoint`` :ref:`option description <geopm-endpoint option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_TIMEOUT``
  The count of seconds that the application will wait for the GEOPM controller
  to connect over shared memory before timing out. See the
  ``--geopm-timeout`` :ref:`option description <geopm-timeout option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_PLUGIN_PATH``
  The colon-separated list of search paths for GEOPM plugins. See the
  ``--geopm-plugin-path`` :ref:`option description <geopm-plugin-path option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_DEBUG_ATTACH``
  An MPI rank number to wait in MPI_Init for a debugger to attach. See the
  ``--geopm-debug-attach`` :ref:`option description <geopm-debug-attach option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_DISABLE_HYPERTHREADS``
  Set to any value to prevent the launcher from pinning to multiple
  hyperthreads per CPU core. See the ``--geopm-hyperthreads-disable``
  :ref:`option description <geopm-hyperthreads-disable option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_OMPT_ENABLE``
  Set to any value to enable OpenMP region detection as described in
  :ref:`geopm.7:integration with ompt`.  See the ``--geopm-ompt-enable``
  :ref:`option description <geopm-ompt-enable option>` in :doc:`geopmlaunch(1)
  <geopmlaunch.1>` for more details.
``GEOPM_INIT_CONTROL``
  The path to the control initialization file.  See the ``--geopm-init-control``
  :ref:`option description <geopm-init-control option>` in
  :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
``GEOPM_PERIOD``
  The control loop period in seconds, if not specified this is determined by
  the Agent. See the ``--geopm-period`` :ref:`option description <geopm-period option>`
  in :doc:`geopmlaunch(1) <geopmlaunch.1>` for details.
``GEOPM_MSR_CONFIG_PATH``
  The colon-separated list of search paths for additional MSR definitions. See
  :doc:`geopm_pio_msr(7) <geopm_pio_msr.7>` for more details.
``GEOPM_CTL_LOCAL``
  Disable communication between controllers running on different
  compute nodes and produce one report file per host.  Enabled by
  default when MPI is not compiled into the GEOPM Runtime.  See the
  ``--geopm-ctl-local`` :ref:`option description <geopm-ctl-local option>`
  in :doc:`geopmlaunch(1) <geopmlaunch.1>` for details.

Other Environment Variables
---------------------------
``LD_DYNAMIC_WEAK``
  When dynamically linking an application to ``libgeopm`` for any
  features supported by the PMPI profiling of the MPI runtime it may
  be required that the ``LD_DYNAMIC_WEAK`` environment variable be set
  at runtime as is documented in the `ld.so(8) <https://man7.org/linux/man-pages/man8/ld.so.8.html>`_ man page.  When
  dynamically linking an application, if care is taken to link the
  ``libgeopm`` library before linking the library providing the weak MPI
  symbols, e.g. ``"-lgeopm -lmpi"``, linking order precedence will
  enforce the required override of the MPI interface symbols and the
  ``LD_DYNAMIC_WEAK`` environment variable is not required at runtime.

Misc
----
:doc:`geopmadmin(1) <geopmadmin.1>`
  Configure and check system wide GEOPM settings
:doc:`geopm_error(3) <geopm_error.3>`
  Error code descriptions
:doc:`geopm_version(3) <geopm_version.3>`
  GEOPM library version
:doc:`geopm_sched(3) <geopm_sched.3>`
  Interface with Linux scheduler
:doc:`geopm_time(3) <geopm_time.3>`
  Time related helper functions
:doc:`geopm_hash(3) <geopm_hash.3>`
  Numerical encoding helper functions

See Also
--------
:doc:`geopmpy(7) <geopmpy.7>`,
:doc:`geopmdpy(7) <geopmdpy.7>`,
:doc:`geopm_agent_frequency_map(7) <geopm_agent_frequency_map.7>`,
:doc:`geopm_agent_ffnet(7) <geopm_agent_ffnet.7>`,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`,
:doc:`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7>`,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`,
:doc:`geopm_agent_frequency_balancer(7) <geopm_agent_frequency_balancer.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopm_pio_const_config(7) <geopm_pio_const_config.7>`,
:doc:`geopm_pio_cnl(7) <geopm_pio_cnl.7>`,
:doc:`geopm_pio_cpuinfo(7) <geopm_pio_cpuinfo.7>`,
:doc:`geopm_pio_dcgm(7) <geopm_pio_dcgm.7>`,
:doc:`geopm_pio_levelzero(7) <geopm_pio_levelzero.7>`,
:doc:`geopm_pio_msr(7) <geopm_pio_msr.7>`,
:doc:`geopm_pio_nvml(7) <geopm_pio_nvml.7>`,
:doc:`geopm_pio_sst(7) <geopm_pio_sst.7>`,
:doc:`geopm_pio_time(7) <geopm_pio_time.7>`,
:doc:`geopm_report(7) <geopm_report.7>`,
:doc:`geopm_agent(3) <geopm_agent.3>`,
:doc:`geopm_ctl(3) <geopm_ctl.3>`,
:doc:`geopm_error(3) <geopm_error.3>`,
:doc:`geopm_field(3) <geopm_field.3>`,
:doc:`geopm_fortran(3) <geopm_fortran.3>`,
:doc:`geopm_hash(3) <geopm_hash.3>`,
:doc:`geopm_policystore(3) <geopm_policystore.3>`,
:doc:`geopm_pio(3) <geopm_pio.3>`,
:doc:`geopm_prof(3) <geopm_prof.3>`,
:doc:`geopm_sched(3) <geopm_sched.3>`,
:doc:`geopm_time(3) <geopm_time.3>`,
:doc:`geopm_version(3) <geopm_version.3>`,
:doc:`geopm::Agent(3) <geopm::Agent.3>`,
:doc:`geopm::Agg(3) <geopm::Agg.3>`,
:doc:`geopm::CircularBuffer(3) <geopm::CircularBuffer.3>`,
:doc:`geopm::CpuinfoIOGroup(3) <geopm::CpuinfoIOGroup.3>`,
:doc:`geopm::Exception(3) <geopm::Exception.3>`,
:doc:`geopm::Helper(3) <geopm::Helper.3>`,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`,
:doc:`geopm::MSRIO(3) <geopm::MSRIO.3>`,
:doc:`geopm::MSRIOGroup(3) <geopm::MSRIOGroup.3>`,
:doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`,
:doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>`,
:doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>`,
:doc:`geopm::PowerBalancer(3) <geopm::PowerBalancer.3>`,
:doc:`geopm::PowerGovernor(3) <geopm::PowerGovernor.3>`,
:doc:`geopm::ProfileIOGroup(3) <geopm::ProfileIOGroup.3>`,
:doc:`geopm::SampleAggregator(3) <geopm::SampleAggregator.3>`,
:doc:`geopm::SharedMemory(3) <geopm::SharedMemory.3>`,
:doc:`geopm::TimeIOGroup(3) <geopm::TimeIOGroup.3>`,
:doc:`geopmadmin(1) <geopmadmin.1>`,
:doc:`geopmagent(1) <geopmagent.1>`,
:doc:`geopmbench(1) <geopmbench.1>`,
:doc:`geopmctl(1) <geopmctl.1>`,
:doc:`geopmlaunch(1) <geopmlaunch.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmaccess(1) <geopmaccess.1>`,
:doc:`geopmexporter(1) <geopmexporter.1>`,
:doc:`geopmsession(1) <geopmsession.1>`,
`ld.so(8) <https://man7.org/linux/man-pages/man8/ld.so.8.html>`_
