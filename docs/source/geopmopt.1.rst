geopmopt(1) -- Bayesian optimization for GEOPM control parameters
=================================================================

Synopsis
--------

.. code-block:: bash

    usage: geopmopt [-h]
                    [--cpu-frequency CPU_FREQUENCY_DOMAIN]
                    [--cpu-frequency-min CPU_FREQUENCY_MIN]
                    [--cpu-frequency-max CPU_FREQUENCY_MAX]
                    [--cpu-frequency-step CPU_FREQUENCY_STEP]
                    [--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN]
                    [--cpu-uncore-frequency-min CPU_UNCORE_FREQUENCY_MIN]
                    [--cpu-uncore-frequency-max CPU_UNCORE_FREQUENCY_MAX]
                    [--cpu-uncore-frequency-step CPU_UNCORE_FREQUENCY_STEP]
                    [--cpu-power CPU_POWER_DOMAIN]
                    [--cpu-power-min CPU_POWER_MIN]
                    [--cpu-power-max CPU_POWER_MAX]
                    [--cpu-power-step CPU_POWER_STEP]
                    [--gpu-frequency GPU_FREQUENCY_DOMAIN]
                    [--gpu-frequency-min GPU_FREQUENCY_MIN]
                    [--gpu-frequency-max GPU_FREQUENCY_MAX]
                    [--gpu-frequency-step GPU_FREQUENCY_STEP]
                    [--gpu-power GPU_POWER_DOMAIN]
                    [--gpu-power-min GPU_POWER_MIN]
                    [--gpu-power-max GPU_POWER_MAX]
                    [--gpu-power-step GPU_POWER_STEP]
                    [--board-power BOARD_POWER_DOMAIN]
                    [--board-power-min BOARD_POWER_MIN]
                    [--board-power-max BOARD_POWER_MAX]
                    [--board-power-step BOARD_POWER_STEP]
                    [--prefetch-disable PREFETCH_DISABLE_DOMAIN]
                    [--prefetch-disable-min PREFETCH_DISABLE_MIN]
                    [--prefetch-disable-max PREFETCH_DISABLE_MAX]
                    [--prefetch-disable-step PREFETCH_DISABLE_STEP]
                    [--trials TRIALS] [--n-initial-points N_INITIAL_POINTS]
                    [--metric-regex METRIC_REGEX] [--minimize]
                    [--random-seed RANDOM_SEED]
                    [--application-timeout APPLICATION_TIMEOUT]
                    [--output-file OUTPUT_FILE] [--verbosity {0,1,2,3}]
                    [--print-stdout] [--defer-write] [--efficiency EFFICIENCY_DOMAIN]
                    [--metric-bound METRIC_BOUND] [--sample-period SAMPLE_PERIOD]
                    [--penalty PENALTY]
                    [-- LAUNCH ...]

Optimize CPU frequency for performance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --verbosity=2 \
             --cpu-frequency board \
             --cpu-uncore-frequency board \
             --metric-regex 'Performance: ([0-9.]+)' \
             --trials 30 \
             -- ./dgemm_bench.sh

Optimize multiple CPU parameters with efficiency focus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --cpu-frequency board \
             --cpu-power board \
             --metric-regex 'Elapsed time: ([0-9.]+)' \
             --minimize \
             --efficiency cpu \
             --trials 30 \
             -- ./mixed_workload.sh

Minimize energy consumption and tune each CPU package independently
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --cpu-frequency package \
             --metric-regex "Energy: ([0-9.]+)" \
             --minimize \
             --trials 30 \
             -- energy_app

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

The tool requires the ``scikit-optimize`` package for Bayesian optimization
functionality: ``python3 -m pip install scikit-optimize``


Options
-------

Control Parameters
~~~~~~~~~~~~~~~~~~

--cpu-frequency CPU_FREQUENCY_DOMAIN  .. _cpu-frequency option:

    Include CPU frequency control in the optimization space for the specified
    domain. The optimizer will explore different CPU frequency settings to
    find optimal performance.

--cpu-frequency-min CPU_FREQUENCY_MIN  .. _cpu-frequency-min option:

    Override the automatically detected CPU frequency minimum before creating the
    optimization grid. This narrows the candidate settings explored by the optimizer.

--cpu-frequency-max CPU_FREQUENCY_MAX  .. _cpu-frequency-max option:

    Override the automatically detected CPU frequency maximum before creating the
    optimization grid. This narrows the candidate settings explored by the optimizer.

--cpu-frequency-step CPU_FREQUENCY_STEP  .. _cpu-frequency-step option:

    Override the step size used to enumerate CPU frequency settings. This controls
    the resolution of the optimization grid.

--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN  .. _cpu-uncore-frequency option:

    Include CPU uncore frequency control in the optimization space. Useful
    for memory-intensive applications where uncore frequency affects performance.

--cpu-uncore-frequency-min CPU_UNCORE_FREQUENCY_MIN  .. _cpu-uncore-frequency-min option:

    Override the automatically detected minimum for CPU uncore frequency when
    constructing the optimization grid.

--cpu-uncore-frequency-max CPU_UNCORE_FREQUENCY_MAX  .. _cpu-uncore-frequency-max option:

    Override the automatically detected maximum for CPU uncore frequency when
    constructing the optimization grid.

--cpu-uncore-frequency-step CPU_UNCORE_FREQUENCY_STEP  .. _cpu-uncore-frequency-step option:

    Override the step size used to enumerate CPU uncore frequency settings.

--cpu-power CPU_POWER_DOMAIN  .. _cpu-power option:

    Include CPU power limit control in the optimization space. Allows the
    optimizer to find optimal power-performance trade-offs.

--cpu-power-min CPU_POWER_MIN  .. _cpu-power-min option:

    Override the automatically detected minimum CPU power limit before the
    optimization grid is created.

--cpu-power-max CPU_POWER_MAX  .. _cpu-power-max option:

    Override the automatically detected maximum CPU power limit before the
    optimization grid is created.

--cpu-power-step CPU_POWER_STEP  .. _cpu-power-step option:

    Override the step size used to enumerate CPU power limit settings.

--gpu-frequency GPU_FREQUENCY_DOMAIN  .. _gpu-frequency option:

    Include GPU frequency control in the optimization space for GPU-accelerated
    applications.

--gpu-frequency-min GPU_FREQUENCY_MIN  .. _gpu-frequency-min option:

    Override the automatically detected minimum GPU frequency prior to building
    the optimization grid.

--gpu-frequency-max GPU_FREQUENCY_MAX  .. _gpu-frequency-max option:

    Override the automatically detected maximum GPU frequency prior to building
    the optimization grid.

--gpu-frequency-step GPU_FREQUENCY_STEP  .. _gpu-frequency-step option:

    Override the step size used to enumerate GPU frequency settings.

--gpu-power GPU_POWER_DOMAIN  .. _gpu-power option:

    Include GPU power limit control in the optimization space.

--gpu-power-min GPU_POWER_MIN  .. _gpu-power-min option:

    Override the automatically detected minimum GPU power limit before the
    optimization grid is created.

--gpu-power-max GPU_POWER_MAX  .. _gpu-power-max option:

    Override the automatically detected maximum GPU power limit before the
    optimization grid is created.

--gpu-power-step GPU_POWER_STEP  .. _gpu-power-step option:

    Override the step size used to enumerate GPU power limit settings.

--board-power board  .. _board-power option:

    Include system-level power limit control in the optimization space for
    comprehensive power management. The only valid domain for this option is
    ``board`` and this option is only available on some platforms that support
    the ``BOARD_POWER_LIMIT_CONTROL`` PlatformIO control.

--board-power-min BOARD_POWER_MIN  .. _board-power-min option:

    Override the automatically detected minimum board-level power limit before
    the optimization grid is created.

--board-power-max BOARD_POWER_MAX  .. _board-power-max option:

    Override the automatically detected maximum board-level power limit before
    the optimization grid is created.

--board-power-step BOARD_POWER_STEP  .. _board-power-step option:

    Override the step size used to enumerate board-level power settings.

--prefetch-disable PREFETCH_DISABLE_DOMAIN  .. _prefetch-disable option:

    Include hardware prefetch disable control in the optimization space. The
    setting progressively disables platform prefetchers as the level increases.
    Valid domains depend on the platform; typically ``board`` or ``package``
    are supported when the underlying MSR controls are exposed.

--prefetch-disable-min PREFETCH_DISABLE_MIN  .. _prefetch-disable-min option:

    Override the minimum prefetch disable level before constructing the
    optimization grid. The default value enables all prefetchers.

--prefetch-disable-max PREFETCH_DISABLE_MAX  .. _prefetch-disable-max option:

    Override the maximum prefetch disable level before constructing the
    optimization grid. The default value disables all supported prefetchers.

--prefetch-disable-step PREFETCH_DISABLE_STEP  .. _prefetch-disable-step option:

    Override the step size used to enumerate prefetch disable levels.

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

--minimize  .. _minimize option:

    Minimize the extracted metric instead of maximizing it. Useful for
    optimizing energy consumption, execution time, or error rates. When
    ``--metric-regex`` is omitted the objective (runtime or energy) is always
    minimized regardless of this flag.

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

Basic CPU frequency optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optimize CPU frequency for a compute-intensive benchmark:

.. code-block:: shell-session

   $ echo '{"loop-count": 300,"region": ["dgemm"],"big-o": [0.1]}' > geopmbench.conf
   $ geopmopt --verbosity=2 \
              --cpu-frequency board \
              --cpu-uncore-frequency board \
              --metric-regex 'Elapsed time: ([0-9.]+)' \
              --minimize \
              --trials 30 \
              -- bash -c "/usr/bin/time -f'Elapsed time: %e' geopmbench geopmbench.conf |& cat"
   INFO: Starting Bayesian optimization with 30 evaluations...
   INFO: Evaluation 1: coordinate=[22, 1], metric=54.33
   INFO: Evaluation 2: coordinate=[21, 4], metric=49.52
   INFO: Evaluation 3: coordinate=[12, 1], metric=54.35
   INFO: Evaluation 4: coordinate=[12, 2], metric=50.78
   ...
   INFO: Evaluation 30: coordinate=[4, 3], metric=51.66
   INFO: Optimization completed!
   INFO: Best metric: 45.81
   INFO: Best coordinate: [18, 6]
   INFO: Number of evaluations: 30
   Best configuration:
   CPU_FREQUENCY_MAX_CONTROL board 0 2800000000.0
   CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 1600000000.0
   CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 1600000000.0

Multi-parameter optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optimize both CPU frequency and power for maximum performance:

.. code-block:: shell-session

   $ echo '{"loop-count": 300,"region": ["dgemm"],"big-o": [0.1]}' > geopmbench.conf
   $ geopmopt --verbosity=2 \
              --cpu-frequency board \
              --cpu-uncore-frequency board \
              --cpu-power board \
              --metric-regex 'Elapsed time: ([0-9.]+)' \
              --minimize \
              --trials 30 \
              -- bash -c "/usr/bin/time -f'Elapsed time: %e' geopmbench geopmbench.conf |& cat"
   INFO: Starting Bayesian optimization with 30 evaluations...
   INFO: Evaluation 1: coordinate=[22, 3, 120], metric=51.0
   INFO: Evaluation 2: coordinate=[16, 6, 15], metric=81.11
   INFO: Evaluation 3: coordinate=[12, 5, 22], metric=77.08
   INFO: Evaluation 4: coordinate=[18, 1, 111], metric=55.49
   ...
   INFO: Evaluation 30: coordinate=[21, 10, 9], metric=85.8
   INFO: Optimization completed!
   INFO: Best metric: 45.05
   INFO: Best coordinate: [27, 14, 154]
   INFO: Number of evaluations: 30
   Best configuration:
   CPU_FREQUENCY_MAX_CONTROL board 0 3700000000.0
   CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 2400000000.0
   CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 2400000000.0
   CPU_POWER_LIMIT_CONTROL board 0 300.0

The resulting configuration file can be applied with:

.. code-block:: shell-session

   $ geopmwrite -f best_config.txt

Minimization optimization
~~~~~~~~~~~~~~~~~~~~~~~~~

Minimize a metric scraped from application output, such as an execution time it
prints itself:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
              --metric-regex "Runtime: ([0-9.]+) seconds" \
              --minimize \
              --trials 40 \
              -- ./timed_benchmark

Minimize runtime without a metric regex
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the application does not print a figure of merit, omit ``--metric-regex``
and ``geopmopt`` minimizes the total wall-clock runtime of the launch command
directly:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
              --cpu-uncore-frequency package \
              --trials 40 \
              -- ./timed_benchmark

Minimize energy without a metric regex
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Combine ``--efficiency`` with no ``--metric-regex`` to minimize total energy
over a domain, measured from a ``geopmsession`` energy trace rather than scraped
from stdout:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
              --cpu-uncore-frequency package \
              --efficiency cpu \
              --sample-period 0.01 \
              --trials 40 \
              -- ./compute_kernel

Bounded-energy optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Minimize energy while holding a performance figure of merit at or above a bound.
Configurations that drop below the bound are treated as infeasible:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
              --metric-regex 'Throughput: ([0-9.]+)' \
              --efficiency cpu \
              --metric-bound 1200.0 \
              --trials 40 \
              -- ./throughput_app

Energy efficiency optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Find the most energy-efficient configuration:

.. code-block:: shell-session

   $ echo '{"loop-count": 300,"region": ["dgemm"],"big-o": [0.1]}' > geopmbench.conf
   $ geopmopt --verbosity=2 \
              --cpu-frequency board \
              --cpu-uncore-frequency board \
              --cpu-power board \
              --metric-regex 'Elapsed time: ([0-9.]+)' \
              --minimize \
              --efficiency cpu \
              --trials 30 \
              -- bash -c "/usr/bin/time -f'Elapsed time: %e' geopmbench geopmbench.conf |& cat"

The ``--efficiency`` flag automatically measures power consumption and optimizes
for operations per watt rather than raw performance.  In the above example the
`--minimize` option is also provided and the reported metric is
time-to-completion, so this will minimize total energy consumed.

Debug mode with application output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use high verbosity and stdout logging for troubleshooting:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
             --metric-regex "Performance: ([0-9.]+)" \
             --verbosity 3 \
             --print-stdout \
             --trials 20 \
             -- ./debug_app

This shows detailed optimization progress and application output to help
debug metric extraction issues.

GPU optimization
~~~~~~~~~~~~~~~~

Optimize GPU parameters for machine learning workloads:

.. code-block:: shell-session

   $ geopmopt --gpu-frequency gpu --gpu-power gpu \
              --metric-regex "Training speed: ([0-9.]+) samples/sec" \
              --trials 60 \
              --application-timeout 600 \
              -- python train_model.py

Complex multi-dimensional optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optimize across all available control dimensions:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package --cpu-power package \
              --gpu-frequency gpu --board-power board \
              --metric-regex "Overall score: ([0-9.]+)" \
              --trials 200 \
              --n-initial-points 20 \
              --random-seed 123 \
              -- ./comprehensive_benchmark


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
