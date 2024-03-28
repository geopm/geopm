geopmlaunch(1) -- application launch wrapper
============================================

Synopsis
--------

.. code-block:: bash

       geopmlaunch launcher [launcher_opt] [geopm_opt] -- executable [executable_opt]

Description
-----------

The ``geopmlaunch`` application enables execution of the GEOPM runtime
along with a distributed parallel compute application, *executable*,
using the command line interface for the underlying application
scheduler, *launcher*, deployed on the HPC system.  The ``geopmlaunch``
command line interface is designed to support many application
schedulers including **SLURM** ``srun``, **ALPS** ``aprun``, and **Intel** / **MPICH**
``mpiexec``.  The ``geopmlaunch`` command line interface has been designed
to wrap the underlying application scheduler while reinterpreting the
command line options, *launcher_opt*, specified for it.  In this way,
the user can easily modify an existing job launch command to enable
the GEOPM runtime by prefixing the command with ``geopmlaunch`` and
extending the existing scheduler options with the options for the
GEOPM runtime, *geopm_opt*.  The GEOPM runtime options control the
behavior of the GEOPM runtime and are detailed in the :ref:`geopmlaunch.1:GEOPM
Options` section below.

All command line options accepted by the underlying job scheduler
(e.g. ``srun`` or ``aprun``) can be passed to the ``geopmlaunch`` wrapper,
except for CPU affinity related options.  The wrapper
script reinterprets the command line to pass modified options and set
environment variables for the underlying application scheduler.  The
GEOPM options, *geopm_opt*, are translated into environment variables
to be interpreted by the GEOPM runtime (see the :ref:`ENVIRONMENT section of
geopm(7) <geopm.7:Environment>`).  The reinterpreted command line, including
the
environment modification, is printed to standard output by the script
before execution.  The command that is printed can be executed in the
``bash`` shell to replicate the execution without using ``geopmlaunch``
directly.  The command is modified to support the GEOPM control thread
by setting CPU affinity for each process and increasing the number of
processes per node or CPUs per process.

.. note::
    The primary compute *executable* and its options,
    *executable_opt*, must appear at the end of the command line and be
    preceded by two dashes: ``--``. The GEOPM launcher will not parse
    arguments to the right of the first ``--`` sequence and will pass the
    arguments that follow unaltered while removing the first ``--`` from the
    command line.  Refer to the :ref:`EXAMPLES <geopmlaunch.1:Examples>` section below.

Supported Launchers
-------------------

The launcher is selected by specifying the *launcher* as the first
command line parameter.  Available *launcher* values are
listed below.


* *srun*, *SrunLauncher*:
  Wrapper for the SLURM resource manager's ``srun`` job launcher.  The
  ``-cc`` and ``--cpu-bind`` options are reserved for use by GEOPM;
  do not specify CPU-binding options when using ``geopmlaunch``.
* *aprun*, *AlpsLauncher*:
  Wrapper for the Cray ALPS ``aprun`` job launcher.  The ``-cc`` and
  ``--cpu-binding`` options are reserved for use by GEOPM; do not
  specify these when using ``geopmlaunch``.
* *impi*, *IMPIExecLauncher*:
  Wrapper for the Intel MPI ``mpiexec`` job launcher.  The
  ``KMP_AFFINITY`` ``I_MPI_PIN_DOMAIN``, and ``MV2_ENABLE_AFFINITY``
  environment variables are reserved for use by GEOPM and are overwritten
  when using ``geopmlaunch``.
* *ompi*, *OMPIExecLauncher*:
  Wrapper for the Open MPI ``mpiexec`` job launcher.  The
  ``-rf`` and ``--rank-file`` as well as explicitly specifying number of
  processes with ``-H`` and ``--host`` options are reserved for use by GEOPM;
  do not specify when using ``geopmlaunch``.
* *SrunTOSSLauncher*:
  Wrapper for ``srun`` when used with the Trilab Operating System
  Software stack.  This special launcher was developed to support
  special affinity plugins for SLURM that were deployed at LLNL's
  computing center.  The ``-cc`` and ``--cpu-bind`` options are
  reserved for use by GEOPM; do not specify when using ``geopmlaunch``.
* *pals*, *PALSLauncher*:
  Wrapper for Cray-HPE PALS launcher, leveraging ``mpiexec``.  The
  ``--cpu-bind`` option is reserved for use by GEOPM; do not specify
  CPU-binding options when using ``geopmlaunch``.

GEOPM Options
-------------

--geopm-report path   .. _geopm-report option:

                      Specifies the path to the GEOPM report output file that
                      is generated at the conclusion of the run if this option
                      is provided.  If the option is not provided, a report
                      named ``geopm.report`` will be created.  The GEOPM report
                      contains a summary of the profiling information collected
                      by GEOPM throughout the application execution.  Refer to
                      :doc:`geopm_report(7) <geopm_report.7>` for a
                      description of the information contained in the report.

                      This option is used by the launcher to set the
                      ``GEOPM_REPORT`` environment variable.  The command line
                      option will override any value currently set in the
                      environment.  See the :ref:`ENVIRONMENT section of
                      geopm(7) <geopm.7:Environment>`.
--geopm-report-signals signals  .. _geopm-report-signals option:

                                Used to insert additional measurements into the
                                report beyond the default values.  See the
                                :ref:`Notes On Sampling section of
                                geopm_report(7) <geopm_report.7:Notes On
                                Sampling>` for more information about how
                                signal samples are summarized over time and
                                across sampling domains in a report.

                                The value of *signals* must be formatted as a
                                comma-separated list of valid signal names.
                                The available signals and their descriptions
                                are documented in the :doc:`geopm_pio(7)
                                <geopm_pio.7>` man page.

                                By default, the signals in the report are
                                aggregated to the board domain.  A domain other
                                than board can be specified by appending the
                                signal name with an ``'@'`` character and then
                                specifying one of the domains.  For example,
                                the following will extend the region and
                                application totals sections of the report with
                                package energy for each package and DRAM energy
                                summed over the all DIMMs:

                                ``--geopm-report-signals=CPU_ENERGY@package,DRAM_ENERGY``

                                The :doc:`geopmread(1) <geopmread.1>`
                                executable enables discovery of signals and
                                domains available on your system.  The signal
                                names and domain names given for this parameter
                                are specified as in the :doc:`geopmread(1)
                                <geopmread.1>` command line interface.
--geopm-trace path              .. _geopm-trace option:

                                The base name and path of the trace file(s)
                                generated if this option is specified.  One
                                trace file is generated for each compute node
                                used by the application containing a
                                pipe-delimited ASCII table describing a time
                                series of measurements made by the GEOPM
                                runtime.  The path is extended with the host
                                name of the node for each created file.  The
                                trace files will be written to the file system
                                path specified or current directory if only a
                                file name is given.  This feature is primarily
                                a debugging tool, and may not scale to large
                                node counts due to file system issues.  This
                                option is used by the launcher to set the
                                ``GEOPM_TRACE`` environment variable.  The command
                                line option will override any value currently
                                set in the environment.  See the
                                :ref:`ENVIRONMENT section of geopm(7)
                                <geopm.7:Environment>`.
--geopm-trace-signals signals   .. _geopm-trace-signals option:

                                Used to insert additional columns into the
                                trace beyond the default columns and the
                                columns added by the Agent.  This option has no
                                effect unless tracing is enabled with
                                ``--geopm-trace``.  The value must be formatted
                                as a comma-separated list of valid signal
                                names.  When not specified all custom signals
                                added to the trace will be sampled and
                                aggregated for the entire node unless the
                                domain is specified by appending ``"@domain_type"``
                                to the signal name.  For example, the following
                                will add total DRAM energy and power as columns
                                in the trace:

                                ``--geopm-trace-signals=DRAM_ENERGY,DRAM_POWER``

                                The signals available and their descriptions
                                are documented in the :doc:`PlatformIO(3)
                                <GEOPM_CXX_MAN_PlatformIO.3>` man page.
                                ``TIME``, ``EPOCH_COUNT``, ``REGION_HASH``,
                                ``REGION_HINT``, ``REGION_PROGRESS``,
                                ``CPU_ENERGY``, ``DRAM_ENERGY``, ``CPU_POWER``,
                                ``DRAM_POWER``, ``CPU_FREQUENCY_STATUS``,
                                ``CPU_CYCLES_THREAD``, ``CPU_CYCLES_REFERENCE``,
                                ``CPU_CORE_TEMPERATURE`` are included in the
                                trace by default.  A domain other than board
                                can be specified by appending the signal name
                                with an ``'@'`` character and then specifying one
                                of the domains, e.g:

                                ``--geopm-trace-signals=CPU_POWER@package,CPU_ENERGY@package``

                                will trace the package power and energy for
                                each package on the system.  The :doc:`geopmread(1)
                                <geopmread.1>` executable enables discovery of
                                signals and domains available on your system.
                                The signal names and domain names given for
                                this parameter are specified as in the
                                :doc:`geopmread(1) <geopmread.1>` command line
                                interface.  This option is used by the launcher
                                to set the ``GEOPM_TRACE_SIGNALS`` environment
                                variable.  The command line option will
                                override any value currently set in the
                                environment.  See the :ref:`ENVIRONMENT section
                                of geopm(7)<geopm.7:Environment>`.
--geopm-trace-profile           .. _geopm-trace-profile option:

                                The base name and path of the profile trace
                                file(s) generated if this option is specified.
                                One trace file is generated for each compute
                                node used by the application containing a
                                pipe-delimited ASCII table describing a log of
                                each call to the ``geopm_prof_*()`` APIs.  The
                                path is extended with the host name of the node
                                for each created file.  The profile trace files
                                will be written to the file system path
                                specified or current directory if only a file
                                name is given.  This feature is primarily a
                                debugging tool, and may not scale to large node
                                counts due to file system issues.  This option
                                is used by the launcher to set the
                                ``GEOPM_TRACE_PROFILE`` environment variable.  The
                                command line option will override any value
                                currently set in the environment.  See the
                                :ref:`ENVIRONMENT section of
                                geopm(7)<geopm.7:Environment>`.
--geopm-trace-endpoint-policy path  .. _geopm-trace-endpoint-policy option:

                                    The path to the endpoint policy trace file
                                    generated if this option is specified.
                                    This file tracks only policies sent through
                                    the endpoint at the root controller, not
                                    all policies within the controller tree.
                                    If ``--geopm-endpoint`` is not provided, or
                                    if the agent does not have any policy
                                    values, this file will not be created.
                                    This option is used by the launcher to set
                                    the ``GEOPM_TRACE_ENDPOINT_POLICY``` environment
                                    variable.  The command line option will
                                    override any value currently set in the
                                    environment.  See the
                                    :ref:`ENVIRONMENT section of
                                    geopm(7)<geopm.7:Environment>`.
--geopm-profile name            .. _geopm-profile option:

                                The name of the profile which is printed in the
                                report and trace files.  This name can be used
                                to index the data in post-processing.  For
                                example, when running a sweep of experiments
                                with multiple power caps, the profile could
                                contain the power setting for one run.  The
                                default profile name is "default".  This option
                                is used by the launcher to set the ``GEOPM_PROFILE``
                                environment variable.  The command line option
                                will override any value currently set in the
                                environment.  See the :ref:`ENVIRONMENT section
                                of geopm(7)<geopm.7:Environment>`.
--geopm-ctl CONTROL_MODE  .. _geopm-ctl option:

                          Use the GEOPM runtime and launch GEOPM with one of
                          three ``CONTROL_MODE``\ s: *application*, *process*,
                          or *pthread*.

                          When used with ``srun``, the *application* method of
                          launch must be called inside an existing allocation
                          made with ``salloc`` or ``sbatch`` and the command
                          must request all the compute nodes assigned to the
                          allocation. This method is the default if the
                          ``--geopm-ctl`` option is not provided.

                          When invoked with non-MPI applications, the *process*
                          and *pthread* methods will silently fail to launch geopm.
                          Only the *application* method will launch geopm with
                          non-MPI applications.

                          The *process* method allocates one extra MPI process
                          per node for the GEOPM controller. The *process* method
                          can be used in the widest variety of cases, but some
                          systems require that each MPI process be assigned the
                          same number of CPUs which may waste resources by
                          assigning more than one CPU to the GEOPM controller
                          process.

                          The *pthread* method spawns a thread from one MPI
                          process per node to run the GEOPM controller.  The
                          *application* method launches the :doc:`geopmctl(1)
                          <geopmctl.1>` application in the background which
                          connects to the primary compute application. The
                          *pthread* option requires support for
                          ``MPI_THREAD_MULTIPLE``, which is not enabled at many
                          sites.

                          The ``--geopm-ctl`` option is used by the launcher to
                          set the ``GEOPM_CTL`` environment variable.  The command
                          line option will override any value currently set in
                          the environment.  See the :ref:`ENVIRONMENT section
                          of geopm(7)<geopm.7:Environment>`.
--geopm-agent agent   .. _geopm-agent option:

                      Specify the Agent type.  The Agent defines the control
                      algorithm used by the GEOPM runtime.  Available agents
                      are: ``"monitor"`` (default, enables profiling features
                      only), ``"power_balancer"`` (optimizes runtime under a power
                      cap), ``"power_governor"`` (enforces a uniform power cap),
                      and ``"frequency_map"`` (runs each region at a specified
                      frequency).  See :doc:`geopm_agent_monitor(7)
                      <geopm_agent_monitor.7>`,
                      :doc:`geopm_agent_power_balancer(7)
                      <geopm_agent_power_balancer.7>`,
                      :doc:`geopm_agent_power_governor(7)
                      <geopm_agent_power_governor.7>`,
                      :doc:`geopm_agent_frequency_map(7)
                      <geopm_agent_frequency_map.7>` and
                      :doc:`geopm_agent_ffnet(7)
                      <geopm_agent_ffnet.7>` for descriptions of each
                      agent.

                      For more details on the responsibilities of an agent,
                      see :doc:`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`.

                      This option is used by the launcher to set the
                      ``GEOPM_AGENT`` environment variable.  The command line
                      option will override any value currently set in the
                      environment.  See the :ref:`ENVIRONMENT section of
                      geopm(7)<geopm.7:Environment>`.
--geopm-policy policy   .. _geopm-policy option:

                        GEOPM policy JSON file used to configure the Agent
                        plugin.  If the policy is provided through this file,
                        it will only be read once and cannot be changed
                        dynamically.  In this mode, samples will not be
                        provided to the resource manager.  See :doc:`geopmagent(1)
                        <geopmagent.1>` and :doc:`geopm_agent(3)
                        <geopm_agent.3>` for more information about how to
                        create this input file.

                        This option is used by the launcher to set the
                        ``GEOPM_POLICY`` environment variable.  The command line
                        option will override any value currently set in the
                        environment.  See the :ref:`ENVIRONMENT section of
                        geopm(7)<geopm.7:Environment>`.

--geopm-init-control path  .. _geopm-init-control option:

                           Initialize any available controls with the values in
                           the file specified by *path*.  The file format is the
                           same as that for ``geopmwrite`` with one control
                           specified per line.  The comment character is '#'.
                           It may be placed anywhere on a line to stop parsing
                           of that line.  If the comment character results in
                           invalid syntax an error will be raised.

                           Files that contain only comments are valid.  They will
                           be parsed but no controls will be written.  A file that
                           is truly empty (i.e. contains only '\0') will raise
                           an error.

                           Example file contents:

                           .. code-block::

                              CPU_POWER_LIMIT_CONTROL board 0 200 # Set a 200W power limit
                              # Also set the time limit
                              CPU_POWER_TIME_WINDOW_CONTROL board 0 0.015

                           This option is used by the launcher to set the
                           ``GEOPM_INIT_CONTROL`` environment variable.  The
                           command line option will override any value currently
                           set in the environment.  See the :ref:`ENVIRONMENT
                           section of geopm(7)<geopm.7:Environment>`.

--geopm-affinity-enable  .. _geopm-affinity-enable option:

                          GEOPM will choose CPU affinity settings to minimize
			  interference between the GEOPM Runtime, the OS, and the
			  application.  When specified, the launcher will emit
			  command line arguments and/or environment variables
			  related to affinity settings for the underlying
			  launcher.  The user should refrain from using of
			  command line options or environment variable that are
			  known to modify application CPU affinity when
			  specifying this option for ``geopmlaunch``.

--geopm-endpoint endpoint  .. _geopm-endpoint option:

                           Prefix for shared memory keys used by the endpoint.
                           The endpoint receives policies dynamically from the
                           resource manager.
                           Refer to :doc:`geopm_endpoint(3)
                           <geopm_endpoint.3>` for more detail.

                           If this option is provided, the GEOPM
                           controller will also send samples to the endpoint at
                           runtime, depending on the Agent selected.  This
                           option overrides the use of ``--geopm-policy`` to
                           receive policy values.  This option is used by the
                           launcher to set the ``GEOPM_ENDPOINT`` environment
                           variable.  The command line option will override any
                           value currently set in the environment.  See the
                           :ref:`ENVIRONMENT section of
                           geopm(7)<geopm.7:Environment>`.
--geopm-timeout sec  .. _geopm-timeout option:

                     Time in seconds that the application should wait for the
                     GEOPM controller to connect over shared memory.  The
                     default value is 30 seconds.  This option is used by the
                     launcher to set the ``GEOPM_TIMEOUT`` environment variable.
                     The command line option will override any value currently
                     set in the environment.  See the :ref:`ENVIRONMENT section
                     of geopm(7)<geopm.7:Environment>`.
--geopm-plugin-path path  .. _geopm-plugin-path option:

                          The search path for GEOPM plugins. It is a
                          colon-separated list of directories used by GEOPM to
                          search for shared objects which contain GEOPM
                          plugins.  In order to be available to the GEOPM
                          runtime, plugins should register themselves with the
                          appropriate factory.  See :doc:`geopm::PluginFactory(3)
                          <GEOPM_CXX_MAN_PluginFactory.3>` for information
                          about the GEOPM plugin interface.

                          A zero-length directory name indicates the current
                          working directory; this can be specified by a leading
                          or trailing colon, or two adjacent colons.  The
                          default search location is always loaded first and is
                          determined at library configuration time and by way
                          of the ``'pkglib'`` variable (typically
                          ``/usr/lib64/geopm/``).  This option is used by the
                          launcher to set the ``GEOPM_PLUGIN_PATH`` environment
                          variable.  The command line option will override any
                          value currently set in the environment.  See the
                          :ref:`ENVIRONMENT section of
                          geopm(7)<geopm.7:Environment>`.
--geopm-record-filter filter  .. _geopm-record-filter option:

                              Applies the user specified filter to the
                              application record data feed.  The filters
                              currently supported are ``"proxy_epoch"`` and
                              ``"edit_distance"``.  These filters can be used to
                              infer the application outer loop (epoch) without
                              modifying the application by inserting calls to
                              ``geopm_prof_epoch()`` (see :doc:`geopm_prof(3)
                              <geopm_prof.3>`).  Region entry and exit may
                              be captured automatically through runtimes such
                              as MPI and OpenMP.

                              The ``"proxy_epoch"`` filter looks for entries into a
                              specific region that serves as a proxy for epoch
                              events.  The filter is specified as a
                              comma-separated list.  The first value selects
                              the filter by name: ``"proxy_epoch"``. The second
                              value in the comma-separated list specifies a
                              region that will be used as a proxy for calls to
                              ``geopm_prof_epoch()``.  If the value can be
                              interpreted as an integer, it will be used as the
                              numerical region hash of the region name,
                              otherwise, the value is interpreted as the region
                              name.  The third value that can be provided in
                              the comma-separated list is optional.  If
                              provided, it specifies the number of region
                              entries into the proxy region that are expected
                              per outer loop.  By default, this is assumed to be
                              1.  The fourth optional parameter that can be
                              specified in the comma-separated list is the
                              number of region entries into the proxy region
                              that are expected prior to the outer loop
                              beginning.  By default, this is assumed to be 0.
                              In the following example, the ``MPI_Barrier`` region
                              entry is used as a proxy for the epoch event:

                              .. code-block::

                                 --geopm-record-filter=proxy_epoch,MPI_Barrier

                              In the next example the ``MPI_Barrier`` region is
                              specified as a hash and the calls per outer loop
                              is given as 6:

                              .. code-block::

                                 --geopm-record-filter=proxy_epoch,0x7b561f45,6


                              In the last example the calls prior to startup is specified as 10:

                              .. code-block::

                                 --geopm-record-filter=proxy_epoch,MPI_Barrier,6,10


                              **Note:** you must specify the calls per outer loop
                              in order to specify the calls prior to startup.

                              The ``"edit_distance"`` filter will attempt to infer
                              the epoch based on patterns in the region entry
                              events using an edit distance algorithm.  The
                              filter is specified as string beginning with the
                              name ``"edit_distance"``; if optional parameters are
                              specified, they are provided as a comma-separated
                              list following the name.  The first parameter is
                              the buffer size; the default if not provided is
                              100.  The second parameter is the minimum stable
                              period length in number of records.  The third
                              parameter is the stable period hysteresis factor.
                              The fourth parameter is the unstable period
                              hysteresis factor.  In the following example, the
                              ``"edit_distance"`` filter will be used with all
                              optional parameters provided:

                              .. code-block::

                                 --geopm-record-filter=edit_distance,200,8,2.0,3.0
--geopm-debug-attach rank  .. _geopm-debug-attach option:

                           Enables a serial debugger such as ``gdb`` to attach to a
                           job when the GEOPM PMPI wrappers are enabled.  If
                           set to a numerical value, the associated rank will
                           wait in ``MPI_Init()`` until a debugger is attached and
                           the local variable ``"cont"`` is set to a non-zero
                           value.  If set, but not to a numerical value then
                           all ranks will wait.  The runtime will print a
                           message explaining the hostname and process ID that
                           the debugger should attach to.  This option is used
                           by the launcher to set the ``GEOPM_DEBUG_ATTACH``
                           environment variable.  The command line option will
                           override any value currently set in the environment.
                           See the :ref:`ENVIRONMENT section of
                           geopm(7)<geopm.7:Environment>`.
--geopm-preload  .. _geopm-preload option:

                Use LD_PRELOAD to link libgeopm.so at runtime.  This
                can be used to enable the GEOPM runtime when an
                application has not been compiled against libgeopm.so.
--geopm-hyperthreads-disable  .. _geopm-hyperthreads-disable option:

                              Prevent the launcher from trying to use
                              hyperthreads for pinning purposes when attempting
                              to satisfy the MPI ranks / OMP threads
                              configuration specified.  This is done for both
                              the controller and the application.  An error is
                              raised if the launcher cannot satisfy the current
                              request without hyperthreads.
--geopm-ctl-disable  .. _geopm-ctl-disable option:

                     Used to allow passing the command through to the
                     underlying launcher.  By default, ``geopmlaunch`` will
                     launch the GEOPM runtime in process mode.  When this
                     option is specified, the GEOPM runtime will not be
                     launched.
--geopm-ctl-local  .. _geopm-ctl-local option:

                   Disable all communication between controllers
                   running on different compute nodes.  This will
                   result in one report file per host, and each report
                   file will have the ``hostname`` appended to the file
                   path requested by the user.  Note this option has no
                   effect if ``--geopm-ctl`` or ``--geopm-ctl-disable``
                   are provided. This is especially useful when launching
                   non-MPI applications.
--geopm-ompt-disable  .. _geopm-ompt-disable option:

                      Disable OMPT detection of OpenMP regions.
                      See the :ref:`INTEGRATION WITH OMPT section of geopm(7)<geopm.7:Integration With OMPT>`
                      for more information about OpenMP region detection.
--geopm-period  .. _geopm-period option:

                Override the control loop period specified by the Agent.  All
                agents have a default control loop period, and this command
                line option allows users to set a different value in units of
                seconds.  With longer control loop periods, the overhead for
                using GEOPM will be reduced, but more interpolation will be
                required when aligning the sparsely sampled hardware signals
                with the application feedback.  Additionally, agent reaction
                time is reduced with longer control loop periods.
--geopm-program-filter  .. _geopm-program-filter option:

                        Only enable profiling for processes where their
                        ``program_invocation_name`` or
                        ``program_invocation_name_short_name`` matches one of the names
                        in the comma separated list provided by
                        option.  This is especially useful when
                        launching a bash script to avoid profiling
                        bash or other ancillary commands that are not
                        part of the main application process set.  See
                        `program_invocation_name(3)` for more details.


Examples
--------

Use ``geopmlaunch`` to queue a job using ``geopmbench`` on a SLURM managed system
requesting two nodes using 32 application MPI process each with four threads:

.. code-block:: bash

   geopmlaunch srun -N 2 -n 32 -c 4 \
                    --geopm-ctl=process \
                    --geopm-report=tutorial6.report \
                    -- ./geopmbench tutorial6_config.json


Use ``geopmlaunch`` to launch the ``miniFE`` executable with the same configuration,
but on an ALPS managed system:

.. code-block:: bash

   geopmlaunch aprun -N 2 -n 64 --cpus-per-pe 4 \
                     --geopm-ctl process \
                     --geopm-report miniFE.report \
                     -- ./miniFE.x -nx 256 -ny 256 -nz 256


Environment
-----------

Every command line option to the launcher can also be specified as an
environment variable if desired (except for ``--geopm-ctl``).
For example, instead of specifying ``--geopm-trace=geopm.trace`` one can
instead set in the environment ``GEOPM_TRACE=geopm.trace`` prior to
invoking the launcher script.  The environment variables are named the
same as the command line option but have the hyphens replaced with
underscores, and are all uppercase.  The command line options take
precedence over the environment variables.

The usage of ``--geopm-ctl`` here is slightly different from how the
controller handles the ``GEOPM_CTL`` environment variable.  In the
case of the launcher, one can specify *process*, *pthread*, or
*application* to the command line argument.  In the case of
``GEOPM_CTL`` one can ONLY specify ``process`` or ``pthread``, as
launching the controller as a separate application is handled through
the ``geopmctl`` binary.

The interpretation of the environment is affected if either of the
GEOPM configuration files exist:

.. code-block::

   /etc/geopm/environment-default.json
   /etc/geopm/environment-override.json


These files may specify system default and override settings for all
of the GEOPM environment variables.  The ``environment-default.json``
file contains a JSON object mapping GEOPM environment variable strings
to strings that define default values for any unspecified GEOPM
environment variable or unspecified ``geopmlaunch`` command line
options.  The ``environment-override.json`` contains a JSON object that
defines values for GEOPM environment variables that take precedence
over any settings provided by the user either through the environment
or through the ``geopmlaunch`` command line options.  The order of
precedence for each GEOPM variable is: override configuration file,
``geopmlaunch`` command line option, environment setting, the default
configuration file, and finally there are some preset default values
that are coded into GEOPM which have the lowest precedence.

The ``KMP_WARNINGS`` environment variable is set to ``'FALSE'``, thus
disabling the Intel OpenMP warnings.  This avoids warnings emitted
because the launcher configures the ``OMP_PROC_BIND`` environment
variable to support applications compiled with a non-Intel
implementation of OpenMP.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopmpy(7) <geopmpy.7>`,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`,
:doc:`geopm_agent_power_balancer(7) <geopm_agent_power_balancer.7>`,
:doc:`geopm_agent_power_governor(7) <geopm_agent_power_governor.7>`,
:doc:`geopm_report(7) <geopm_report.7>`,
:doc:`geopm_error(3) <geopm_error.3>`,
:doc:`geopmctl(1) <geopmctl.1>`
