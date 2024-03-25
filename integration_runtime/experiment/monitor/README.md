MONITOR-BASED EXPERIMENTS
-------------------------

The scripts in this directory are used to run applications with the
monitor agent and analyze some basic runtime characteristics of
applications.

## Base Experiment Module

#### `monitor.py`:

  Contains helper functions for launching.  When launched with
  these helpers, the report will be extended with some additional
  signals that can be used by analysis scripts.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_plot_achieved_power.py`:

  Plots a histogram of the achieved power on each package from an
  unconstrained run.