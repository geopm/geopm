geopmopt(1) -- Bayesian optimization for GEOPM control parameters
=================================================================

Synopsis
--------

.. code-block:: bash

   usage: geopmopt [-h] [--cpu-frequency CPU_FREQUENCY_DOMAIN]
                   [--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN]
                   [--cpu-power CPU_POWER_DOMAIN]
                   [--gpu-frequency GPU_FREQUENCY_DOMAIN]
                   [--gpu-power GPU_POWER_DOMAIN]
                   [--board-power BOARD_POWER_DOMAIN]
                   [--trials TRIALS] [--n-initial-points N_INITIAL_POINTS]
                   --metric-regex METRIC_REGEX [--minimize] [--random-seed RANDOM_SEED]
                   [--application-timeout APPLICATION_TIMEOUT]
                   [--output-file OUTPUT_FILE] [--verbosity {0,1,2,3}]
                   [--print-stdout] [--defer-write] [--efficiency EFFICENCY_DOMAIN]
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

4. **Metric Extraction**: Parses application output using Python regular
   expressions to extract numeric performance metrics.

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

--cpu-uncore-frequency CPU_UNCORE_FREQUENCY_DOMAIN  .. _cpu-uncore-frequency option:

    Include CPU uncore frequency control in the optimization space. Useful
    for memory-intensive applications where uncore frequency affects performance.

--cpu-power CPU_POWER_DOMAIN  .. _cpu-power option:

    Include CPU power limit control in the optimization space. Allows the
    optimizer to find optimal power-performance trade-offs.

--gpu-frequency GPU_FREQUENCY_DOMAIN  .. _gpu-frequency option:

    Include GPU frequency control in the optimization space for GPU-accelerated
    applications.

--gpu-power GPU_POWER_DOMAIN  .. _gpu-power option:

    Include GPU power limit control in the optimization space.

--board-power board  .. _board-power option:

    Include system-level power limit control in the optimization space for
    comprehensive power management. The only valid domain for this option is
    ``board`` and this option is only available on some platforms that support
    the ``BOARD_POWER_LIMIT_CONTROL`` PlatformIO control.

Optimization Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

--trials TRIALS  .. _trials option:

    Number of optimization iterations to perform. More trials generally lead to
    better results but take longer. Default: 50.

--n-initial-points N_INITIAL_POINTS  .. _n-initial-points option:

    Number of random initial evaluations before starting Bayesian optimization.
    These provide initial data for the Gaussian Process model. Default: 10.

--metric-regex METRIC_REGEX  .. _metric-regex option:

    Python-style regular expression to extract the performance metric from
    application output. The regex must capture the numeric value in a group.
    Required option.

--minimize  .. _minimize option:

    Minimize the extracted metric instead of maximizing it. Useful for
    optimizing energy consumption, execution time, or error rates.

--random-seed RANDOM_SEED  .. _random-seed option:

    Random seed for reproducible optimization results. Default: 42.

--application-timeout APPLICATION_TIMEOUT  .. _application-timeout option:

    Timeout in seconds for application execution. Applications exceeding this
    timeout are terminated. Default: 300.

--efficiency EFFICIENCY_DOMAIN  .. _efficiency option:

    Optimize for efficiency by dividing the extracted metric by average power
    consumption. This finds configurations that maximize performance per watt.
    When the ``--minimize option`` is provided, the average power consumption is
    multiplied rather than divided.  This will minimize energy to completion if
    the metric is time to completion.  The EFFICIENCY_DOMAIN determines the
    components included in the power calculation and valid values are 'board',
    'cpu', or 'gpu'.

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

Minimize execution time or energy consumption:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
              --metric-regex "Runtime: ([0-9.]+) seconds" \
              --minimize \
              --trials 40 \
              -- ./timed_benchmark

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

The ``--metric-regex`` option uses Python regular expressions to extract
performance metrics from application output. The regex must capture the
numeric value in a parenthesized group.

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

The optimizer handles various failure scenarios:

**Application Timeouts:** Applications exceeding ``--application-timeout`` are
terminated and receive a penalty score.

**Configuration Errors:** Invalid control parameters are detected early using
the GEOPM PIO interface.

**Regex Failures:** Missing or invalid metric patterns are reported with
suggestions for debugging.

**Application Failures:** Non-zero exit codes and stderr output are captured
and logged.

**Optimization Failures:** Issues with the Bayesian optimization algorithm
are reported with diagnostic information.


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
combined with ``geopmlauch --geopm-init-control`` to obtain per region metrics
or distribute write commands across a multi-node allocation.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmgrid(1) <geopmgrid.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmsession(1) <geopmsession.1>`
