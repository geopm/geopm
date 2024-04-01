
geopm_report(7) -- GEOPM summary report file
============================================

Description
-----------

The GEOPM report is a **YAML** file containing summarized information
about runtimes, network usage, energy consumption, and other data
about an application's behavior on every compute node where it ran.
The report is broken into sections with totals for each compute node,
and further divided into sections detailing each application region
and epoch.  By default, the report will be saved into a file called
``"geopm.report"``, but this name can be customized with the ``GEOPM_REPORT``
environment variable; refer to the description for ``--geopm-report`` in
:doc:`geopmlaunch(1) <geopmlaunch.1>`.

The application regions and epoch are defined by use of the
:doc:`geopm_prof(3) <geopm_prof.3>` interface to mark up the user application, or
through automatic inference of regions based on interposing on the MPI
or OpenMP interfaces (interposing on OpenMP requires that OMPT is
enabled at GEOPM compile time, and the ``--geopm-ompt-disable`` option
is not provided to ``geopmlaunch``). Alternatively, epochs may be
inserted automatically when ``--geopm-record-filter`` is used, as
described in :doc:`geopmlaunch(1) <geopmlaunch.1>`.

Notes On Sampling
-----------------
Most data in the report is derived from :doc:`PlatformIO <geopm::PlatformIO.3>` signals (described
in :doc:`geopm_pio(7) <geopm_pio.7>`) sampled at a rate determined by the
``Agent::wait()`` implementation for the active agent (see
:doc:`geopm::Agent(3) <geopm::Agent.3>`).  If a sample is longer than the rate specified
by the agent, the subsequent sample may be shortened to correct the rate.

GEOPM reports summarize sampled signals across time and across sampling
domains. A signal is summarized across time by displaying a single value in a
report even though the many values may have been observed at different sample
times. A signal is summarized across domains when the signal is sampled at a
lower-level domain (e.g., CPU-level) than the displayed domain (e.g.,
board-level).

Aggregation Over Time
^^^^^^^^^^^^^^^^^^^^^
Signals are summarized over time following a method that depends on the signal's
*behavior*, as declared by ``signal_behavior()`` in :doc:`geopm::IOGroup(3)
<geopm::IOGroup.3>`.

If a signal is declared as monotonically increasing, then the reported value
is the amount of change measured in that signal.

If a signal is not monotonically increasing, then the report shows
time-weighted mean of the sampled values.

Aggregation Across Domains
^^^^^^^^^^^^^^^^^^^^^^^^^^
GEOPM supports reporting signals in the signal's native domain or in a more
coarse domains. If a more coarse domain is requested, the native signal is
aggregated across domains as defined by the signal's ``agg_function``,
documented in :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`. Common
aggregation functions are documented in :doc:`geopm::Agg(3) <geopm::Agg.3>`.

Attributing Signals to Regions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Sample calculations (regardless of time-aggregation method) are attributed to
the current region hash for the corresponding domain.  By default,
synchronous values in the report are aggregated to the board domain,
so deltas will be attributed to a region only when all active CPUs on
the compute node are in that region.

For example, synchronous time is aggregated into the ``sync-runtime (s)`` field
as the sum of all ``delta(time)`` when all active cores on the node were in the
same region.  User extensions through ``GEOPM_REPORT_SIGNALS`` may be aggregated to
smaller domains.  If any CPUs within the domain are in different regions at the
time of the sample, the delta value will be attributed to the unmarked region.
Only active CPUs assigned to the application are considered when determining
the region to attribute values to.

Values that are the result of sampling (most fields except ``runtime``
and ``count``) may include small contributions from adjacent regions,
especially if regions are very short.  The amount of slop depends on
the controller sample rate and the number of region entries.  Sampled
values should not be compared with the per-process average runtime
``runtime (s)``; use ``sync-runtime (s)`` instead.

Refer to the :ref:`EXAMPLES <geopm_report.7:Examples>` section below.

Report Format
-------------

Header Fields
^^^^^^^^^^^^^
GEOPM Version
  The version of the GEOPM library and tools used for the run.  Note that if
  building from source, ``./autogen.sh`` must be rerun to update the version to
  the latest git SHA.
Start Time
  The start time of the job recorded by the GEOPM controller.
Profile
  The profile string, obtained from the ``GEOPM_PROFILE`` environment variable.
Agent
  The name of the agent, obtained from the ``GEOPM_AGENT`` environment variable.
Policy
  A JSON string containing the policy values as set in ``GEOPM_POLICY``, or
  ``"DYNAMIC"`` if the policy was received through the endpoint.

Per-host Region, Epoch, and Application Totals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Under the ``Hosts`` section of the report, each host in the job creates
a section with its hostname as the key.  Each host's section contains:

* ``Application Totals``, the totals for each metric from the beginning to
  the end of the job
* ``Epoch Totals``, the totals from the first detected epoch to the end of the job
* ``Regions``, the summarized data for each region.

The fields in each of these sections are described below:

``name``
  Name of the region.  For user-defined regions, this is the string passed to
  ``geopm_region()``; for automatically detected OpenMP and MPI regions this
  is the function name.  See :doc:`geopm_prof(3) <geopm_prof.3>` for more
  information.

``hash``
  The hash of the region name.  This value is used by agents to distinguish
  regions using the ``REGION_HASH`` signal and also appears in the trace.

``runtime (s)``
  The average across all processes of the total runtime spent in the region.
  For epoch totals, this is the time from the first detected epoch to the end
  of the application.  For application totals, this is the time from the start
  to the end of the application, corresponding to the ``MPI_Init()`` and
  ``MPI_Finalize()`` calls.  This ``runtime`` is more accurate than ``sync-runtime``
  because it uses exact entry and exit timestamps rather than the sampled
  ``REGION_HASH``.  It should not be used for comparisons with sampled values
  below; use ``sync-runtime`` for comparisons instead.

``count``
  The total number of times this region was entered and exited, averaged
  across all processes.  Fractional counts are possible if some processes
  entered a region a different number of times.  For epoch totals, this is the
  total number of detected epochs, averaged across all processes.  The count
  has no meaning for the unmarked region and application totals.

``sync-runtime (s)``
  Total time for which the sampled region hash matched this region on all CPUs
  on the compute node.  For epoch and application totals, this value is the
  same as ``runtime (s)``.  The ``sync-runtime`` for the unmarked region represents
  the total time for which the ranks on a compute node were not in the same
  region (i.e. unaligned).  All subsequent default fields in the report are
  sampled in the same way as the ``sync-runtime`` and can be compared with it.

``package-energy (J)``
  Total energy in *joules* consumed by all processor packages (sockets).

``dram-energy (J)``
  Total energy in *joules* consumed by all DRAM on the board.

``power (W)``
  Average power for the processor package, calculated as ``package-energy`` divided
  by ``sync-runtime``.

``frequency (%)``
  Achieved core frequency as a *percentage* of the sticker (base) frequency for
  the processor.  This frequency is calculated using the ratio of ``CPU_CYCLES_THREAD``
  to ``CPU_CYCLES_REFERENCE``.

``frequency (Hz)``
  Achieved core frequency for the processor in *hertz*.  This frequency is
  calculated using the ratio of ``CPU_CYCLES_THREAD`` to ``CPU_CYCLES_REFERENCE`` times the
  sticker (base) frequency.

``time-hint-network (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_NETWORK``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-ignore (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_IGNORE``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-compute (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_COMPUTE``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-memory (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_MEMORY``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-io (s)``
  The portion of ``sync-runtime`` where the region hint was ``GEOPM_REGION_HINT_IO``.
  The region hint is determined by the hint passed to ``geopm_region()`` for
  the most nested region.

``time-hint-serial (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_SERIAL``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-parallel (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_PARALLEL``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-unknown (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_UNKNOWN``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``time-hint-unset (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_UNSET``.  The region hint also becomes unset when exiting an
  unnested region (i.e. when a CPU is in the unmarked region).

``time-hint-spin (s)``
  The portion of ``sync-runtime`` where the region hint was
  ``GEOPM_REGION_HINT_SPIN``.  The region hint is determined by the hint passed
  to ``geopm_region()`` for the most nested region.

``gpu-energy (J)``
  Total energy in *joules* consumed by all GPUs.

``gpu-power (W)``
  Average power for the GPUs in *watts*.

``gpu-frequency (Hz)``
  Achieved frequency for the GPUs in *hertz*.

**Report Extensions**
  The report can be extended by agents, or by through the
  ``--geopm-report-signals`` option to ``geopmlaunch`` which corresponds to
  the ``GEOPM_REPORT_SIGNALS`` environment variable.  See the description
  for ``--geopm-report-signals`` in :doc:`geopmlaunch(1) <geopmlaunch.1>` for more details.
  Signals in the ``GEOPM_REPORT_SIGNALS`` list will be added to individual
  regions, the epoch totals, and the application totals for each host.

  Agents can add keys to the report header, host section, or individual
  region sections using the ``report_header()``, ``report_host()`` or
  ``report_region()`` methods respectively.  See :doc:`geopm::Agent(3) <geopm::Agent.3>` for
  more information about the report extensions available to agents.

Examples
--------

Suppose we have a system with 2 sockets per node and 2 CPUs per
package (socket); assume 1 CPU per core for simplicity
(i.e. hyperthreading is turned off if applicable).  The application
places one process (MPI rank) on each CPU, and each rank executes the
same application code, containing regions called ``A`` and ``B``.  The
processes are assigned to cores as follows:

.. code-block::

          socket 0     |      socket 1
      CPU 0  |  CPU 1  |  CPU 2  |  CPU 3
    ---------+---------+---------+----------
        1    |    2    |    3    |    4

The following is an example of the region enter/exit and epoch events
seen by the ``Controller``.  This stream of events will be used to
calculate the average runtime and count over ranks for regions and
epoch, and the current region hash for sampling.

The following is an example of the regions being entered and exited on
each CPU.  The sample rate is ``0.002 s``; the region hash for the
``REGION_HASH`` sample at larger domains is shown on the right side of the
table; ``-`` indicates the unmarked region.  Note that in a real report,
the entry and exit times used to calculate the ``runtime`` may not line
up exactly with the sample boundaries (e.g. ``0.003`` or ``0.005``), and
samples may not be exactly the same length throughout the run.

.. code-block::

     time    CPU0   CPU1   CPU2   CPU3   ||  board   pkg0   pkg1
    -------------------------------------++----------------------
    0.001     -      -      -      -     ||
    0.002     A      -      -      A     ||   -       -      -
    0.003     A      A      -      A     ||
    0.004     A      A      A      A     ||   A       A      A
    0.005     -      A      A      A     ||
    0.006     B      A      A      A     ||   -       -      A
    0.007     B      B      A      B     ||
    0.008     B      B      B      B     ||   B       B      B
    0.009     B      B      B      B     ||
    0.010     B      B      B      B     ||   B       B      B
    0.011     -      -      -      -     ||
    0.012     finalize: report generated ||   -       -      -

A subset of the report is shown below.  The ``runtime`` and ``count``
fields are averaged across the 4 CPUs.  The user extensions for
``TIME@package`` correspond to the ``sync-runtime`` for each package.

Regions:
--------

.. code-block::

   "region": "A",
   "runtime": 0.00375,   // average of [0.002, 0.004, 0.004, 0.005]
   "count": 1,
   "sync-runtime": 0.002,    // 1 sample in A for board
   "TIME@package-0": 0.002,  // 1 sample in A for package 0
   "TIME@package-1": 0.004   // 2 samples in A for package 1
   -
   "region": "B",
   "runtime": 0.002875,  // average of [0.005, 0.004, 0.003, 0.004]
   "count": 1,
   "sync-runtime": 0.004,    // 2 samples in B for board
   "TIME@package-0": 0.004,  // 2 samples in B for package 0
   "TIME@package-1": 0.004,  // 2 samples in B for package 1

  Unmarked Totals:
    "runtime": 0.003,     // average of [0.003, 0.003, 0.004, 0.002]
    "count": 0,
    "sync-runtime": 0.006,    // 3 samples in unmarked for board
    "TIME@package-0": 0.006,  // 3 samples in unmarked for package 0
    "TIME@package-1": 0.004,  // 2 samples in unmarked for package 1

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_prof(3) <geopm_prof.3>`,
:doc:`geopm::Agent(3) <geopm::Agent.3>`,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
