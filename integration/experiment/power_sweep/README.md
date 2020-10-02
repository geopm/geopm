POWER SWEEP-BASED EXPERIMENTS
-----------------------------

The scripts in this directory are used to run and analyze application
runs that use different package power limits as their agent policy.

## Base Experiment Module

#### `power_sweep.py`:

  Contains helper functions for launching a power sweep and common
  command line arguments.

  In addition to command line arguments common to all run scripts,
  power sweep runs also accept the following options:

  - `--max-power`: the maximum power setting for the sweep.  By
                   default, uses TDP for the compute nodes.  Within
                   each trial, the experiment script will start at the
                   highest power limit and run at each limit until
                   it reaches the minimum.

  - `--min-power`: the minimum power setting to use for the sweep.

  - `--step-power`: the step size in watts between settings for the sweep.

  - `--agent-list`: the list of agents to use for the sweep.  They
                    must use a power limit policy.  By default, both
                    the power_governor and power_balancer are run.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### gen_power_sweep_summary.py:

  Prints basic summary of the runtime and energy characteristics at
  each power cap.

#### gen_balancer_comparison.py

  Compares the total runtime and energy of the application when run
  with the power balancer against the power governor and prints the
  runtime and energy savings.

#### gen_plot_node_efficiency.py

  Calculates the achieved frequency of every package of every node at
  each power cap and agent in the experiment.  Produces a histogram
  plot for each power cap and agent summarizing the data.  If
  --show-details is passed, it also prints the same data to standard
  output.

#### gen_plot_balancer_power_limit.py

  Plot over time showing epoch runtimes and power limits chosen by the
  balancer algorithm.  Useful for checking the algorithm behavior.

#### gen_plot_balancer_comparison.py

  Create a bar chart comparing the total runtime or energy of the
  application when run with the power balancer against the power
  governor over a range of power caps.
