geopmopt(1) -- Bayesian optimization for GEOPM control parameters
=================================================================

Synopsis
--------

.. code-block:: bash

    usage: geopmopt [-h]
                    [--sweep DIM]
                    [--list-controls]
                    [--trials TRIALS] [--n-initial-points N_INITIAL_POINTS]
                    [--metric-regex METRIC_REGEX] [--minimize [NAME]]
                    [--metric NAME=SOURCE] [--maximize NAME]
                    [--constraint 'NAME OP VALUE'] [--energy-domain DOMAIN]
                    [--list-metrics]
                    [--random-seed RANDOM_SEED]
                    [--application-timeout APPLICATION_TIMEOUT]
                    [--output-file OUTPUT_FILE] [--verbosity {0,1,2,3}]
                    [--print-stdout] [--defer-write] [--efficiency EFFICIENCY_DOMAIN]
                    [--metric-bound METRIC_BOUND] [--sample-period SAMPLE_PERIOD]
                    [--penalty PENALTY]
                    [-- LAUNCH ...]

List available controls
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --list-controls

List available metrics
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --list-metrics \
             --metric power=signal:CPU_POWER@board:mean

Minimize runtime
~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --sweep cpu-freq@board \
             --trials 30 \
             -- ./workload.sh

Maximize a figure of merit
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --verbosity=2 \
             --sweep cpu-freq@board \
             --sweep uncore-freq@board \
             --metric-regex 'Performance: ([0-9.]+)' \
             --trials 30 \
             -- ./dgemm_bench.sh

Optimize energy efficiency
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --sweep cpu-freq@board \
             --sweep cpu-power@board \
             --efficiency cpu \
             --trials 30 \
             -- ./mixed_workload.sh

Compose an objective with constraints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --sweep cpu-freq@board \
             --metric fom=regex:'GFLOPS: ([0-9.]+)' \
             --metric power=signal:CPU_POWER@board:mean \
             --maximize fom \
             --constraint 'power <= 250W' \
             -- ./app

Get Help
~~~~~~~~

.. code-block:: bash

    geopmopt -h
    geopmopt --help


Description
-----------

Command line interface for Bayesian optimization of GEOPM control parameters.
The tool uses advanced optimization algorithms to automatically find optimal
control settings that maximize (or minimize) application performance metrics.

The optimizer works by:

1. **Parameter Space Definition**: Uses the same grid system as ``geopmgrid``
   to define the search space for control parameters.

2. **Application Evaluation**: Launches the specified application with different
   control configurations and extracts performance metrics from its output.

3. **Bayesian Optimization**: Uses Gaussian Process models and acquisition
   functions to intelligently explore the parameter space, focusing on
   promising regions.

4. **Metric Extraction**: Optionally parses application output using Python
   regular expressions to extract a numeric performance metric. When no
   ``--metric-regex`` is supplied the objective defaults to total wall-clock
   runtime, or to total energy over the ``--efficiency`` domain when that option
   is also given.

Beyond the single-metric ``--metric-regex`` workflow, the general objective
interface (``--metric``, ``--maximize``/``--minimize NAME``, and
``--constraint``) composes an objective from several named metrics and adds
feasibility constraints; see `Objective and Constraint Grammar`_.

The tool requires the optional ``optimize`` dependencies (``scikit-optimize``
and ``pyyaml``), which are not installed by a base ``geopmdpy`` install or by
the OS packages. Install ``geopmdpy`` with the ``optimize`` extra, ideally into
a personal virtual environment (see :ref:`Installing client tools with pip
<install:Installing client tools with pip>`): ``python3 -m pip install
'geopmdpy[optimize]'``


Options
-------

Control Parameters
~~~~~~~~~~~~~~~~~~

--sweep DIM  .. _sweep option:

    Add a control dimension to the optimization search space. May be given
    multiple times to optimize over several controls at once. Each ``DIM`` uses
    the grammar ``CONTROL[@DOMAIN][=MIN:MAX:STEP]``:

    - ``CONTROL`` is a control name or alias (see ``--list-controls`` for the
      full catalog). Recognized names include ``cpu-freq`` (alias
      ``cpu-frequency``), ``uncore-freq`` (alias ``cpu-uncore-frequency``),
      ``cpu-power``, ``gpu-freq`` (alias ``gpu-frequency``), ``gpu-power``,
      ``board-power``, and ``prefetch`` (alias ``prefetch-disable``).

    - ``@DOMAIN`` optionally pins the control to a platform domain such as
      ``board``, ``package``, ``core``, ``cpu``, ``gpu``, or ``gpu_chip``. When
      omitted, the control's native domain is used.

    - ``=MIN:MAX:STEP`` optionally overrides the auto-detected range to narrow
      the candidate settings the optimizer explores. Each field is independent
      and may be left empty to keep its auto-detected value, for example
      ``=1.2GHz:3GHz:100MHz`` (all three), ``=::100MHz`` (step only),
      ``=1.2GHz:3GHz`` (bounds only), or ``=1.2GHz::`` (minimum only).

    Frequency values accept the unit suffixes ``Hz``, ``kHz``, ``MHz``, and
    ``GHz``; power values accept ``W`` and ``kW``. A bare number is interpreted
    in the control's canonical unit (Hz for frequency, W for power). The
    ``prefetch`` control takes non-negative integer levels and rejects unit
    suffixes.

--list-controls  .. _list-controls option:

    Print a table of the available control names, their native domain, units,
    and the detected minimum, maximum, and step values, then exit. Controls
    whose range cannot be read on the current platform are shown as ``n/a``.

Optimization Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

--trials TRIALS  .. _trials option:

    Number of optimization iterations to perform. More trials generally lead to
    better results but take longer. Default: 50.

--n-initial-points N_INITIAL_POINTS  .. _n-initial-points option:

    Number of random initial evaluations before starting Bayesian optimization.
    These provide initial data for the Gaussian Process model. Default: 10.

--metric-regex METRIC_REGEX  .. _metric-regex option:

    Python-style regular expression to extract the figure of merit from the
    application's standard output. The regex must capture the numeric value in a
    group. Optional: when omitted, the objective defaults to total wall-clock
    runtime, or to total energy over the ``--efficiency`` domain when that option
    is also provided.

--minimize [NAME]  .. _minimize option:

    Without an argument, minimize the legacy ``--metric-regex`` figure of merit
    instead of maximizing it (useful for execution time or error rates). When
    ``--metric-regex`` is omitted the objective (runtime or energy) is always
    minimized regardless of this flag.

    With a metric ``NAME`` (part of the general objective interface described
    below), select that ``--metric`` as the objective to minimize. This form is
    mutually exclusive with ``--maximize`` and with the bare legacy usage.

--random-seed RANDOM_SEED  .. _random-seed option:

    Random seed for reproducible optimization results. Default: 42.

--application-timeout APPLICATION_TIMEOUT  .. _application-timeout option:

    Timeout in seconds for application execution. Applications exceeding this
    timeout are terminated. Default: 300.

--efficiency EFFICIENCY_DOMAIN  .. _efficiency option:

    Optimize for efficiency using the average power consumed over the specified
    domain. When ``--metric-regex`` is provided, the extracted metric is divided
    by average power (multiplied instead when ``--minimize`` is set, which
    minimizes energy to completion if the metric is time to completion). When
    ``--metric-regex`` is omitted, the objective becomes total energy over this
    domain, measured directly from a ``geopmsession`` energy trace. The
    EFFICIENCY_DOMAIN determines the components included in the power calculation
    and valid values are ``board``, ``cpu``, or ``gpu``.

--metric-bound METRIC_BOUND  .. _metric-bound option:

    Minimize energy over the ``--efficiency`` domain subject to keeping the
    ``--metric-regex`` figure of merit at or above this bound (at or below when
    ``--minimize`` is set). This turns the run into a constrained optimization:
    configurations that violate the bound are treated as infeasible and only the
    feasible configuration with the lowest energy is reported. Requires both
    ``--metric-regex`` and ``--efficiency``.

--sample-period SAMPLE_PERIOD  .. _sample-period option:

    ``geopmsession`` sampling period in seconds used when collecting the energy
    trace for energy-based objectives (``--efficiency`` without a metric regex,
    or ``--metric-bound``). Shorter periods improve energy-integration accuracy
    at the cost of additional sampling overhead. Default: 0.01.

--penalty PENALTY  .. _penalty option:

    How to handle a *recoverable* trial failure (an application timeout, a
    non-zero exit code, a missing or unparsable figure of merit, or a
    non-positive power/runtime report). The value is one of:

    ``auto`` (default)
        Assign the failed trial an objective value strictly worse than every
        successful trial observed so far, so the optimizer learns to avoid that
        region and the run continues. If a failure occurs before any success,
        a finite bootstrap penalty is used and a warning is logged.

    ``none``
        Abort the whole run on the first failed trial (the legacy behavior).

    ``<number>``
        Use this fixed numeric objective value for every failed trial.

    Fatal errors (command not found, permission denied, configuration or report
    parsing errors) always abort the run regardless of this setting.

General Objective Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~

These options provide a composable alternative to the legacy
``--metric-regex``/``--efficiency``/``--metric-bound`` flags: name any number of
metrics, pick one to optimize, and add feasibility constraints. The legacy flags
remain supported as documented aliases (see `Objective and Constraint
Grammar`_), but may not be combined with the general flags on a single
invocation.

--metric NAME=SOURCE  .. _metric option:

    Define a named metric. ``NAME`` is an identifier matching
    ``[A-Za-z_][A-Za-z0-9_]*``, referenced verbatim by
    ``--maximize``/``--minimize``/``--constraint``. ``SOURCE`` is one of the
    providers ``regex:'PATTERN'``, ``signal:SIGNAL@DOMAIN[:AGG]``, or
    ``expr:'EXPRESSION'``. Repeatable. See `Objective and Constraint Grammar`_
    for the provider grammar, the reserved metric names, and the
    behavior-derived default aggregation.

--maximize NAME  .. _maximize option:

    Select the ``--metric`` named ``NAME`` as the objective to maximize.
    Mutually exclusive with ``--minimize NAME``. Naming an undefined metric is
    an error.

--constraint 'NAME OP VALUE'  .. _constraint option:

    Add a feasibility constraint. ``OP`` is one of ``<=``, ``>=``, ``<``,
    ``>``, ``==``. ``VALUE`` may carry a unit suffix that is normalized to the
    metric's canonical unit (for example ``250W`` or ``0.25kW`` for power,
    ``5000J`` for energy); a bare number is taken to already be in canonical
    units. ``NAME`` must be a defined or reserved metric. Repeatable;
    infeasible configurations are excluded from best-configuration selection.

--energy-domain DOMAIN  .. _energy-domain option:

    Sample energy and average power over ``DOMAIN`` (``board``, ``cpu``, or
    ``gpu``) with ``geopmsession`` so the reserved ``power`` and ``energy``
    metrics are populated on the general interface. This is the general-flag
    counterpart to the legacy ``--efficiency`` and is required before ``power``
    or ``energy`` may be named by ``--maximize``/``--minimize``/``--constraint``
    or referenced by an ``expr:`` metric (for example ``eff=expr:'fom /
    power'``); otherwise those references fail at parse time with a message
    naming the metric. Alternatively, define the quantity explicitly as a
    ``signal:`` metric (for example ``power=signal:CPU_POWER@board:mean``) and
    omit this flag.

--list-metrics  .. _list-metrics option:

    Print the reserved canonical metrics with their units and the mode in which
    each becomes available, followed by each ``signal:`` metric defined via
    ``--metric`` with its behavior and default aggregation, then exit. Signals
    unavailable on the current platform (for example GPU signals on a GPU-less
    node) are shown as ``n/a``. This is the objective-side analogue of
    ``--list-controls``.

Output and Logging
~~~~~~~~~~~~~~~~~~

--output-file OUTPUT_FILE  .. _output-file option:

    Write the best configuration to a file in geopmwrite format. Use '-' for
    stdout (default). The configuration can be applied later with
    ``geopmwrite --config`` or through ``geopmlaunch --geopm-init-control``.

--verbosity {0,1,2,3}  .. _verbosity option:

    Control logging verbosity: 0=ERROR, 1=WARNING, 2=INFO, 3=DEBUG.
    Default: 1.

--print-stdout  .. _print-stdout option:

    Print application stdout to the log at info level. Useful for debugging
    metric extraction or application issues.

--defer-write  .. _defer-write option:

    Defer writing control configurations to another tool such as ``geopmwrite``
    or ``geopmlaunch --geopm-init-control``. This is especially useful for
    running in a distributed environment or avoiding configuration conflicts
    between process sessions. Requires ``--output-file`` which is updated prior
    to each application trial and then after the last trial the optimal
    configuration is written to the same file path.

Application Launch
~~~~~~~~~~~~~~~~~~

LAUNCH ...  .. _launch option:

    Command and arguments to launch the application for evaluation. These may be
    specified after a double dash (``--``) to avoid any parser option
    conflicts. The application should produce the target metric in its standard
    output.  To generate the target metric, it may be useful to wrap the
    application in a bash script that derives and prints the figure of merit.

-h, --help  .. _help option:

    Print help message and exit.


Examples
--------

The examples below progress from the simplest single-control run to the general
multi-metric objective interface.  Each assumes ``scikit-optimize`` is installed
and that a live GEOPM service grants write access to the swept controls.

Minimize wall-clock runtime
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The simplest run sweeps a single control and, with no ``--metric-regex``,
minimizes the launch command's wall-clock runtime directly -- the application
need not print a figure of merit:

.. code-block:: shell-session

   $ echo '{"loop-count": 300,"region": ["dgemm"],"big-o": [0.1]}' > geopmbench.conf
   $ geopmopt --verbosity=2 \
              --sweep cpu-freq@board \
              --trials 30 \
              -- geopmbench geopmbench.conf
   INFO: Starting Bayesian optimization with 30 evaluations...
   INFO: Evaluation 1: coordinate=[22], score=54.33
   INFO: Evaluation 2: coordinate=[21], score=49.52
   ...
   INFO: Evaluation 30: coordinate=[18], score=45.81
   INFO: Optimization completed!
   INFO: Best metric: 45.81
   INFO: Best coordinate: [18]
   INFO: Number of evaluations: 30
   Best configuration:
   CPU_FREQUENCY_MAX_CONTROL board 0 2800000000.0

Tune several controls together
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Pass ``--sweep`` more than once to optimize several controls jointly.  Here CPU
frequency, uncore frequency, and the CPU power limit are tuned together, still
minimizing runtime, and the best configuration is written to a file for reuse:

.. code-block:: shell-session

   $ geopmopt --verbosity=2 \
              --sweep cpu-freq@board \
              --sweep uncore-freq@board \
              --sweep cpu-power@board \
              --trials 30 \
              --output-file best_config.txt \
              -- geopmbench geopmbench.conf
   INFO: Starting Bayesian optimization with 30 evaluations...
   INFO: Evaluation 1: coordinate=[22, 3, 120], score=51.0
   ...
   INFO: Optimization completed!
   INFO: Best metric: 45.05
   INFO: Best coordinate: [27, 14, 154]
   INFO: Number of evaluations: 30
   Best configuration:
   CPU_FREQUENCY_MAX_CONTROL board 0 3700000000.0
   CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 2400000000.0
   CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 2400000000.0
   CPU_POWER_LIMIT_CONTROL board 0 300.0

Apply the saved configuration to the current session at any time with:

.. code-block:: shell-session

   $ geopmwrite -f best_config.txt

Maximize an application figure of merit
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the application prints a figure of merit, scrape it with
``--metric-regex``; the captured value is maximized by default:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@board \
              --sweep uncore-freq@board \
              --metric-regex 'GFLOPS: ([0-9.]+)' \
              --trials 30 \
              -- ./dgemm_bench.sh

Minimize a value the application prints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add ``--minimize`` to minimize the scraped value instead, for a quantity such as
an execution time the application reports itself:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@package \
              --metric-regex 'Runtime: ([0-9.]+) seconds' \
              --minimize \
              --trials 40 \
              -- ./timed_benchmark

Optimize for energy efficiency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add ``--efficiency`` to fold measured power into the objective.  With a
``--metric-regex`` the objective becomes the figure of merit per watt
(performance per watt), measured from a ``geopmsession`` energy trace:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@board \
              --sweep uncore-freq@board \
              --metric-regex 'Throughput: ([0-9.]+)' \
              --efficiency cpu \
              --trials 30 \
              -- ./throughput_app

Minimize total energy
~~~~~~~~~~~~~~~~~~~~~

With ``--efficiency`` and no ``--metric-regex`` the objective is the total
energy consumed over the domain, so ``geopmopt`` finds the lowest-energy
configuration:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@package \
              --sweep uncore-freq@package \
              --efficiency cpu \
              --trials 40 \
              -- ./compute_kernel

Compose an objective from named metrics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The general objective interface names any number of metrics, selects one to
optimize, and adds feasibility constraints.  Here the scraped figure of merit is
maximized while measured CPU power and energy are held within bounds; each
``signal:`` metric is sampled with ``geopmsession`` around every trial:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@board --sweep cpu-power@board \
              --metric fom=regex:'GFLOPS: ([0-9.]+)' \
              --metric power=signal:CPU_POWER@board:mean \
              --metric energy=signal:CPU_ENERGY@board:delta \
              --maximize fom \
              --constraint 'power <= 250W' \
              --constraint 'energy <= 5000J' \
              -- ./app

Configurations that exceed 250 W or 5000 J are treated as infeasible, and the
reported best configuration is the feasible one with the largest ``fom``.  See
`Objective and Constraint Grammar`_ for the full provider and constraint syntax.

Optimize a derived metric
~~~~~~~~~~~~~~~~~~~~~~~~~

An ``expr:`` metric combines previously defined metrics with restricted
arithmetic, so a custom objective can be expressed directly instead of through
the ``--efficiency`` shorthand:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@board \
              --metric fom=regex:'GFLOPS: ([0-9.]+)' \
              --metric power=signal:CPU_POWER@board:mean \
              --metric eff=expr:'fom / power' \
              --maximize eff \
              -- ./app

Optimize tokens per watt with an SLA
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Selecting a sampling domain with ``--energy-domain`` populates the reserved
``power`` metric, so no separate ``signal:`` definition is needed. Here
throughput per watt is maximized while a tail-latency service-level objective is
enforced directly by the optimizer, so no client-side latency filtering is
required:

.. code-block:: shell-session

   $ geopmopt --sweep gpu-freq@gpu \
              --energy-domain board \
              --metric tps=regex:'tokens_per_second: ([0-9.]+)' \
              --metric p99=regex:'p99_latency_ms: ([0-9.]+)' \
              --metric tpw=expr:'tps / power' \
              --maximize tpw \
              --constraint 'p99 <= 2000' \
              -- ./serve_bench.sh

Because ``--energy-domain board`` is set, ``power`` is measured from the
``geopmsession`` report and ``tpw`` resolves to ``tps / power`` per trial;
configurations whose ``p99`` exceeds the bound are treated as infeasible.

Preview reserved and referenced metrics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``--list-metrics`` prints the reserved canonical metrics (each annotated with
the mode in which it becomes available) and every ``signal:`` metric referenced
by ``--metric`` -- each with its behavior and default aggregation -- then exits
without running the application.  Use it to confirm a signal is available and
how it will be aggregated before launching a long run (the objective-side
analogue of ``--list-controls``):

.. code-block:: shell-session

   $ geopmopt --list-metrics \
              --metric power=signal:CPU_POWER@board:mean \
              --metric energy=signal:CPU_ENERGY@board
   METRIC      UNIT  AVAILABILITY
   time        s     always measured
   energy      J     needs --energy-domain (or --efficiency), or a signal: metric
   power       W     needs --energy-domain (or --efficiency), or a signal: metric
   fom         arb   needs a regex: metric (or --metric-regex)

   SIGNAL                  DOMAIN    BEHAVIOR    AGGREGATION
   CPU_POWER               board     variable    mean
   CPU_ENERGY              board     monotone    delta

Optimize GPU parameters
~~~~~~~~~~~~~~~~~~~~~~~

Sweep GPU controls the same way as CPU controls; a longer
``--application-timeout`` accommodates slower launches:

.. code-block:: shell-session

   $ geopmopt --sweep gpu-freq@gpu --sweep gpu-power@gpu \
              --metric-regex 'Training speed: ([0-9.]+) samples/sec' \
              --trials 60 \
              --application-timeout 600 \
              -- python3 train_model.py

Defer applying the configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

With ``--defer-write`` ``geopmopt`` does not apply candidate controls itself;
instead it writes each candidate to ``--output-file`` before its trial so
another tool applies it.  This suits distributed runs and integration with
``geopmlaunch --geopm-init-control``:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@board \
              --metric-regex 'GFLOPS: ([0-9.]+)' \
              --defer-write \
              --output-file candidate.txt \
              --trials 30 \
              -- ./app

Troubleshoot metric extraction
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Raise the verbosity and echo application stdout to debug a regex that is not
matching:

.. code-block:: shell-session

   $ geopmopt --sweep cpu-freq@package \
              --metric-regex 'Performance: ([0-9.]+)' \
              --verbosity 3 \
              --print-stdout \
              --trials 20 \
              -- ./debug_app


Metric Extraction
-----------------

The optional ``--metric-regex`` option uses Python regular expressions to
extract performance metrics from application output. When supplied, the regex
must capture the numeric value in a parenthesized group. When omitted, no
scraping is performed and the objective defaults to runtime (or energy with
``--efficiency``).

Valid regex examples:

.. code-block:: text

   "GFLOPS: ([0-9.]+)"                    # Floating point after "GFLOPS: "
   "Time: ([0-9]+) seconds"               # Integer time value
   "Score: ([0-9]*\.?[0-9]+)"             # Decimal with optional point
   "Throughput: ([0-9.]+e[+-]?[0-9]+)"    # Scientific notation
   "Performance: ([0-9,]+\.?[0-9]*)"      # Numbers with commas

The extracted value is automatically converted to a floating-point number
for optimization.


Objective and Constraint Grammar
--------------------------------

The general objective interface (``--metric``, ``--maximize``,
``--minimize NAME``, ``--constraint``, and ``--list-metrics``) composes a single
scalar objective from named metrics and optional feasibility constraints. It is
the objective-side analogue of the ``--sweep`` search-space grammar, and it
subsumes the legacy ``--metric-regex``/``--efficiency``/``--metric-bound`` flags,
which are retained as documented aliases (see `Legacy flag aliases`_). The two
interfaces may not be combined on a single invocation.

Named metrics
~~~~~~~~~~~~~

Each ``--metric NAME=SOURCE`` defines one metric. ``NAME`` is an identifier
matching ``[A-Za-z_][A-Za-z0-9_]*`` and is referenced verbatim by
``--maximize``/``--minimize``/``--constraint``. ``SOURCE`` is one of three
providers:

.. list-table::
   :header-rows: 1
   :widths: 15 30 55

   * - Provider
     - Grammar
     - Meaning
   * - ``regex:``
     - ``regex:'PATTERN'``
     - Float scraped from application stdout (capture group 1, or the whole
       match). The same scraper as ``--metric-regex``.
   * - ``signal:``
     - ``signal:SIGNAL@DOMAIN[:AGG]``
     - A GEOPM signal sampled around the run via ``geopmsession`` and reduced by
       ``AGG`` (one of ``delta``, ``mean``, ``max``, ``min``). ``AGG`` defaults
       from the signal behavior (see below).
   * - ``expr:``
     - ``expr:'EXPRESSION'``
     - A derived metric over previously defined metric names, e.g.
       ``expr:'energy / fom'``. Evaluated by a restricted arithmetic AST
       evaluator, never ``eval()``.

.. note::

   The ``@DOMAIN`` of a ``signal:`` metric and the ``--energy-domain`` value are
   *sampling* domains -- ``board``, ``cpu``, or ``gpu`` -- over which a signal is
   read and reduced. They are distinct from the *control* (write) domains
   attached to ``--sweep`` (``package``, ``core``, ``cpu``, ``gpu``,
   ``gpu_chip``, ...), which select where a control setting is applied; a single
   invocation may sweep a control on one domain while sampling a metric on
   another.

Reserved metric names
~~~~~~~~~~~~~~~~~~~~~

A small set of canonical names carry a known unit so a constraint spelling is
unambiguous. They may be referenced by ``--constraint`` and selected as the
objective without a ``--metric`` definition. Each is populated only in the mode
noted below; referencing one the current invocation does not measure fails at
parse time with a message naming the metric and how to enable it (see
``--energy-domain`` and ``--list-metrics``):

.. list-table::
   :header-rows: 1
   :widths: 20 15 65

   * - Metric
     - Unit
     - Meaning
   * - ``time``
     - ``s``
     - Wall-clock/report runtime. Always available; may not be redefined by
       ``--metric``.
   * - ``energy``
     - ``J``
     - Energy consumed during the run. Populated only with ``--energy-domain``
       (or legacy ``--efficiency``); otherwise define it as a ``signal:``
       metric.
   * - ``power``
     - ``W``
     - Average power over the run. Populated only with ``--energy-domain`` (or
       legacy ``--efficiency``); otherwise define it as a ``signal:`` metric.
   * - ``fom``
     - ``arb``
     - Figure of merit scraped from stdout (dimensionless). Populated only when
       a ``regex:`` metric (or legacy ``--metric-regex``) supplies it.

Default aggregation from signal behavior
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a ``signal:`` metric omits ``:AGG``, the aggregation is derived from the
signal's reported behavior rather than hard-coded per name. An explicit ``:AGG``
always overrides this default. Constant and label signals carry no per-trial
quantity and are rejected.

.. list-table::
   :header-rows: 1
   :widths: 25 20 55

   * - Signal behavior
     - Default ``AGG``
     - Example signals
   * - Monotone (counter that only increases)
     - ``delta``
     - ``CPU_ENERGY``, ``TIME``, instruction/clock counters
   * - Variable (fluctuates up and down)
     - ``mean``
     - ``CPU_POWER``, ``CPU_FREQUENCY_STATUS``, temperature

Selecting the objective
~~~~~~~~~~~~~~~~~~~~~~~

``--maximize NAME`` and ``--minimize NAME`` select exactly one objective metric
and its direction. Providing both, or naming an undefined metric, is an error.
When neither is given, the objective defaults to ``minimize time`` (the
wall-clock fallback).

Constraints
~~~~~~~~~~~

``--constraint 'NAME OP VALUE'`` (repeatable) adds a feasibility bound. ``OP`` is
one of ``<=``, ``>=``, ``<``, ``>``, ``==``. ``VALUE`` may carry a unit suffix
that is normalized to the metric's canonical unit (for example ``250W`` or
``0.25kW`` for a power metric, ``5000J`` for energy); a bare number is taken to
already be in canonical units. ``NAME`` must be a defined or reserved metric.
Constraints are folded into the single scalar objective through a normalized
penalty so heterogeneous units remain commensurable, and the reported best
configuration is the feasible one that best optimizes the objective.

Legacy flag aliases
~~~~~~~~~~~~~~~~~~~

The v1 objective flags remain supported and are expanded internally into the
canonical grammar above; they are mutually exclusive with the general flags on
one invocation. The canonical spellings below are illustrative (``<power>`` and
``<energy>`` stand for the domain's power/energy signals):

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Legacy invocation
     - Canonical equivalent
   * - ``--metric-regex P``
     - ``--metric fom=regex:P --maximize fom``
   * - ``--metric-regex P --minimize``
     - ``--metric fom=regex:P --minimize fom``
   * - (no ``--metric-regex``)
     - ``--minimize time``
   * - ``--efficiency D`` (with regex ``P``)
     - ``--metric fom=regex:P --metric power=signal:<power>@D:mean --metric eff=expr:'fom / power' --maximize eff``
   * - ``--efficiency D --minimize`` (no regex)
     - ``--metric energy=signal:<energy>@D:delta --minimize energy``
   * - ``--metric-bound B`` (+ regex + efficiency)
     - ``--minimize energy --constraint 'fom >= B'`` (``<=`` when ``--minimize``)


Optimization Algorithm
----------------------

The tool uses Gaussian Process-based Bayesian optimization from scikit-optimize:

**Gaussian Process Model:** Learns a probabilistic model of the objective
function from previous evaluations.

**Acquisition Function:** Uses Expected Improvement (EI) to balance exploration
of uncertain regions with exploitation of promising areas.

**Initial Sampling:** Starts with random evaluations to build initial model data.

**Convergence:** Iteratively refines the model and selects the most promising
configurations to evaluate next.

This approach is much more efficient than grid search or random search,
especially for expensive function evaluations.


Error Handling
--------------

The optimizer sorts evaluation failures into two categories.

**Recoverable failures** do not abort the run. By default (``--penalty auto``)
the failed trial is assigned an objective worse than every success, is logged at
warning level, and is excluded from best-configuration selection, so a single
flaky trial no longer discards the measurements already collected. The
recoverable set is:

- **Application Timeouts:** Applications exceeding ``--application-timeout`` are
  terminated and the trial is penalized.
- **Application Failures:** Non-zero exit codes (with captured stderr) penalize
  the trial.
- **Metric Scrape Misses:** A ``--metric-regex`` that does not match, or a
  matched value that cannot be parsed as a number, penalizes the trial.
- **No Output:** A trial that produces no stdout when a metric is expected is
  penalized.
- **Bad Energy Report:** A ``geopmsession`` energy trace with non-positive
  runtime or non-positive power penalizes the trial.

Use ``--penalty none`` to restore the legacy behavior of aborting on the first
recoverable failure, or ``--penalty <number>`` to assign a fixed penalty value.

**Fatal errors** always abort the run regardless of ``--penalty``:

- **Missing Command / Permissions:** A launch command that cannot be found or
  executed.
- **Configuration Errors:** Invalid control parameters, detected early using the
  GEOPM PIO interface.
- **Report Parsing Errors:** A missing or malformed ``geopmsession`` report.
- **Optimization Failures:** Issues with the Bayesian optimization algorithm
  are reported with diagnostic information.

If *every* trial fails, the run stops with a clear "All trials failed" error
rather than emitting a meaningless configuration.


Best Practices
--------------

**Start Small:** Begin with 20-30 trials to validate your setup before running
longer optimizations.

**Validate Metrics:** Test your regex pattern on sample application output
before starting optimization.

**Choose Appropriate Domains:** Select control domains that have measurable
impact on your application's performance.

**Monitor Progress:** Use verbosity level 2 or higher to watch optimization
convergence.

**Set Reasonable Timeouts:** Allow enough time for application execution but
prevent runaway processes.

**Use Seeds:** Set ``--random-seed`` for reproducible experiments and
comparison studies.

**Baseline First:** Run your application without optimization to establish
baseline performance metrics.


Integration
-----------

The ``geopmopt`` tool integrates with the broader GEOPM ecosystem:

**ControlGrid Integration:** Uses the same parameter space definition as
``geopmgrid`` for consistency.

**GEOPM Service:** Leverages the PIO interface for hardware control and
energy measurement.

**Configuration Output:** Generates standard geopmwrite configurations that can
be saved and reused.

**Session Monitoring:** Can be combined with ``geopmsession`` for detailed
performance analysis during optimization.

**GEOPM Runtime:** When using the ``--defer-write`` option ``geopmopt`` can be
combined with ``geopmlaunch --geopm-init-control`` to obtain per region metrics
or distribute write commands across a multi-node allocation.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmgrid(1) <geopmgrid.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmsession(1) <geopmsession.1>`
