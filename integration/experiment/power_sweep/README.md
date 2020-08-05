POWER SWEEP-BASED EXPERIMENTS
-----------------------------

The scripts in this directory are used to run and analyze application
runs that use different package power limits as their agent policy.

The general workflow is as follows:

1. Run the sweep for a specific application with
   run_power_sweep_<app>.py.  An directory to hold all output files is
   recommended and can be specified with `--output-dir`.  The number
   of nodes can be specified with `--nodes`; otherwise 1 node will be
   used.

2. Generate the desired analysis targetting the resulting output with
   `--output-dir`; by default all reports/traces in the current
   working directory are used.  Some scripts support more verbose
   output with `--show-details`.

Example:

1. Run DGEMM on 4 nodes:

  ./run_power_sweep_dgemm.py --nodes=4 --output-dir=test42

2. Calculate the achieved frequency on each node and generate histogram plots:

  ./node_efficiency.py --output-dir=test42



Base Experiment Module
----------------------
power_sweep.py:

  - Contains the helper function for launching a power sweep.


Application Specific Run Scripts
--------------------------------

run_power_sweep_dgemm.py:

  - Runs DGEMM on a number of nodes over a range of power caps.

run_power_sweep_dgemm_tiny.py

  - Runs a small DGEMM problem on a number of nodes over a range of
    power caps.  This will run more quickly than the default dgemm.

Analysis Scripts to Produce Summary Tables and Visualizations
-------------------------------------------------------------

gen_power_sweep_summary.py:

  - Prints basic summary of the runtime and energy characteristics at
    each power cap.

gen_balancer_comparison.py

  - Compares the total runtime and energy of the application when run
    with the power balancer against the power governor and prints the
    runtime and energy savings.

gen_node_efficiency.py

  - Calculates the achieved frequency of every package of every node
    at each power cap and agent in the experiment.  Produces a
    histogram plot for each power cap and agent summarizing the data.
    If --show-details is passed, it also prints the same data to
    standard output.

gen_plot_balancer_power_limit.py

  - Plot over time showing epoch runtimes and power limits chosen by the
    balancer algorithm.  Useful for checking the algorithm behavior.