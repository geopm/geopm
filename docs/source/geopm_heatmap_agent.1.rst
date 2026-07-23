geopm_heatmap_agent(1) -- render a component-saturation heatmap from GEOPM telemetry
====================================================================================

Synopsis
--------

.. code-block:: bash

    python3 -m geopmdpy.heatmap_agent [-t TIME] [-p PERIOD] [-o TRACE_OUT]
                                      [-i CONFIG_PATH] [--heatmap-out PATH]
                                      [--fom FOM_CSV] [--no-plot]
                                      [--replot-csv TRACE_CSV]

Sample and render (default)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    python3 -m geopmdpy.heatmap_agent -t 30 --heatmap-out heatmap.png

Correlate against a figure of merit
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    python3 -m geopmdpy.heatmap_agent -t 60 --heatmap-out hm.png --fom fom.csv

Re-render a saved trace without sampling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    python3 -m geopmdpy.heatmap_agent -t 60 -o trace.csv --no-plot
    python3 -m geopmdpy.heatmap_agent --replot-csv trace.csv --heatmap-out hm.png


Description
-----------

The ``heatmap_agent`` is a pure-Python implementation of the
:doc:`geopm::Agent(3) <geopm::Agent.3>` interface for the GEOPM Python session
(see :doc:`geopmsession(1) <geopmsession.1>`).  It samples a set of
per-component signals in-process through the ``geopmdpy.pio`` interface and,
when the session ends, renders a normalized *component-saturation heatmap* with
one row per component and time along the horizontal axis.  Each row is scaled to
its own dynamic range, so a component pinned near its own maximum while the
others sit idle reads out as the bottleneck for that phase of the run.

The agent samples the following signals when they are granted on the platform;
any that are unavailable are silently dropped:

* ``CPU_POWER`` (CPU package power)
* ``MSR::CPU_SCALABILITY_RATIO`` (stalled-vs-retiring proxy)
* ``CPU_FREQUENCY_STATUS`` (core frequency)
* ``CPU_UNCORE_FREQUENCY_STATUS`` (uncore frequency)
* ``DRAM_POWER`` (memory power)
* ``GPU_POWER``, ``GPU_CORE_ACTIVITY``, ``GPU_UNCORE_ACTIVITY``,
  ``GPU_UTILIZATION`` (GPU power and activity, when a GPU IOGroup is present)

When both ``CPU_INSTRUCTIONS_RETIRED`` and ``CPU_CYCLES_THREAD`` are available,
the agent derives a per-sample **IPC** row from the deltas of the two cumulative
counters and also emits it as an ``IPC`` column in the trace.  High IPC alongside
modest DRAM and uncore activity indicates a compute-bound region; low IPC with
saturated DRAM power or uncore frequency indicates a memory-bound region.

Because this agent extends the session interface, it accepts all of the standard
:doc:`geopmsession(1) <geopmsession.1>` options (for example ``-t/--time``,
``-p/--period``, ``-o/--trace-out``, and ``-i/--signal-config``) in addition to
the options listed below.  The agent supplies its own default signal request
set, so no request list needs to be piped in; provide ``-i CONFIG_PATH`` to
trace a custom set of requests instead.  The default sampling period is lowered
to *0.05 second*; an explicit ``-p/--period`` still takes precedence.  This is a
read-only monitoring agent and writes no controls.


Options
-------

--heatmap-out PATH  .. _heatmapout option:

    Output PNG path for the rendered heatmap.  Default:
    ``component_heatmap.png``.

--fom FOM_CSV  .. _fom option:

    Overlay an external figure-of-merit time series beneath the heatmap.  The
    CSV must have two columns, ``time_seconds`` and ``value``.  The FOM axis
    shares the heatmap's time axis to aid correlation of the metric against the
    telemetry.

--no-plot  .. _noplot option:

    Collect the trace but skip rendering the heatmap.  Useful for long or
    high-rate captures; render the resulting trace later with ``--replot-csv``.

--replot-csv TRACE_CSV  .. _replotcsv option:

    Render a heatmap from an existing ``geopmsession`` trace CSV and exit without
    opening a session.  No PlatformIO access is required.  The PNG is written to
    ``--heatmap-out`` and ``--fom`` may be combined with it.  When the trace
    contains the ``CPU_INSTRUCTIONS_RETIRED`` and ``CPU_CYCLES_THREAD`` counters
    the IPC row is derived from them; otherwise an existing ``IPC`` column is
    used if present.  This option is distinct from ``-f/--report-format``, which
    selects the report output format.


Requirements
------------

Rendering requires ``matplotlib`` (and ``pandas`` for the ``--fom`` overlay and
``--replot-csv`` modes).  These are optional dependencies of ``geopmdpy`` and can
be installed with:

.. code-block:: bash

    pip install geopmdpy[heatmap]

Importing the module and running a ``--no-plot`` capture do not require these
packages.


Limitations
-----------

GEOPM exposes no NIC telemetry and has no notion of data *flowing* between
components, so this tool visualizes per-component saturation over time rather
than a data-flow diagram between the CPU, GPU, and other devices.  When
rendering at the end of a run (the default), samples are accumulated in memory
for the duration of the session; for very long or high-rate captures prefer
``--no-plot`` and re-render the saved trace with ``--replot-csv``.


See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopmdpy(7) <geopmdpy.7>`\ ,
:doc:`geopmsession(1) <geopmsession.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`
