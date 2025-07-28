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
                   [--print-stdout] [--defer-write] [--efficiency]
                   [-- LAUNCH ...]

Optimize CPU frequency for performance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --cpu-frequency package --metric-regex "GFLOPS: ([0-9.]+)" \
             --trials 50 -- ./benchmark

Optimize multiple parameters with efficiency focus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --cpu-frequency package --cpu-power board \
             --metric-regex "Performance: ([0-9.]+)" \
             --efficiency --trials 100 -- python ml_training.py

Minimize energy consumption
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmopt --cpu-frequency package --metric-regex "Energy: ([0-9.]+)" \
             --minimize --trials 30 -- ./energy_app

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
functionality: ``pip install scikit-optimize``


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

    Include GPU power limit control in the optimization space. The tool
    automatically detects Intel Level Zero or NVIDIA NVML interfaces.

--board-power BOARD_POWER_DOMAIN  .. _board-power option:

    Include system-level power limit control in the optimization space for
    comprehensive power management.

Optimization Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

--trials TRIALS  .. _trials option:

    Number of optimization iterations to perform. More trials generally lead
    to better results but take longer. Default: 50.

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

--efficiency  .. _efficiency option:

    Optimize for efficiency by dividing the extracted metric by average power
    consumption. This finds configurations that maximize performance per watt.

Output and Logging
~~~~~~~~~~~~~~~~~~

--output-file OUTPUT_FILE  .. _output-file option:

    Write the best configuration to a file in geopmwrite format. Use '-' for
    stdout (default). The configuration can be applied later with geopmwrite.

--verbosity {0,1,2,3}  .. _verbosity option:

    Control logging verbosity: 0=ERROR, 1=WARNING, 2=INFO, 3=DEBUG.
    Default: 1.

--print-stdout  .. _print-stdout option:

    Print application stdout to the log at info level. Useful for debugging
    metric extraction or application issues.

--defer-write  .. _defer-write option:

    Defer writing control configurations until the end of optimization.
    Requires ``--output-file``. Useful for avoiding configuration conflicts
    during optimization.

Application Launch
~~~~~~~~~~~~~~~~~~

LAUNCH ...  .. _launch option:

    Command and arguments to launch the application for evaluation. Specified
    after a double dash (``--``). The application should produce the target
    metric in its standard output.

-h, --help  .. _help option:

    Print help message and exit.


Examples
--------

Basic CPU frequency optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optimize CPU frequency for a compute-intensive benchmark:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
             --metric-regex "GFLOPS: ([0-9.]+)" \
             --trials 30 \
             -- ./stream_benchmark
   INFO: Starting Bayesian optimization with 30 evaluations...
   INFO: Evaluation 1: coordinate=[10], metric=45.2
   INFO: Evaluation 2: coordinate=[15], metric=48.7
   ...
   INFO: Optimization completed!
   INFO: Best metric: 52.3
   INFO: Best coordinate: [18]
   Best configuration:
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 0 2800000000
   geopmwrite CPU_FREQUENCY_MAX_CONTROL package 1 2800000000

Multi-parameter optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Optimize both CPU frequency and power for balanced performance:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package --cpu-power board \
             --metric-regex "Score: ([0-9.]+)" \
             --trials 100 \
             --output-file best_config.txt \
             -- python ml_workload.py
   INFO: Starting Bayesian optimization with 100 evaluations...
   ...
   INFO: Best configuration written to best_config.txt

The resulting configuration file can be applied with:

.. code-block:: shell-session

   $ geopmwrite -c best_config.txt

Energy efficiency optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Find the most energy-efficient configuration:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
             --metric-regex "Operations: ([0-9]+)" \
             --efficiency \
             --trials 50 \
             -- ./compute_workload
   INFO: Starting Bayesian optimization with 50 evaluations...
   INFO: Evaluation 1: coordinate=[8], metric=245.6 (efficiency mode)
   ...

The ``--efficiency`` flag automatically measures power consumption and optimizes
for operations per watt rather than raw performance.

Minimization optimization
~~~~~~~~~~~~~~~~~~~~~~~~~

Minimize execution time or energy consumption:

.. code-block:: shell-session

   $ geopmopt --cpu-frequency package \
             --metric-regex "Runtime: ([0-9.]+) seconds" \
             --minimize \
             --trials 40 \
             -- ./timed_benchmark

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

**Configuration Output:** Generates standard geopmwrite commands that can
be saved and reused.

**Session Monitoring:** Can be combined with ``geopmsession`` for detailed
performance analysis during optimization.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmgrid(1) <geopmgrid.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmsession(1) <geopmsession.1>`
