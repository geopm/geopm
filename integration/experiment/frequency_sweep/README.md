Frequency Sweep-Based Experiments
---------------------------------

The scripts in this directory are used to run and analyze application
runs that use a range of fixed frequency settings with the frequency
map agent.

## Base Experiment Module

#### `frequency_sweep.py`:

  Contains the helper function for launching a frequency sweep and
  common command line arguments.

  In addition to command line arguments common to all run scripts,
  frequency sweep runs also accept the following options:

  - `--max-frequency`: the maximum frequency setting for the sweep.
                       By default, it uses the sticker for the compute
                       nodes, and executes an additional run with max
                       turbo frequency as the limit.  Within each
                       trial, the experiment script will start at the
                       highest frequency and run at each limit until
                       it reaches the minimum.

  - `--min-frequency`: the minimum frequency setting for the sweep.

  - `--step-frequency`: the step size in hertz between settings for the sweep.

  - `--run-max-turbo`: executes an additional run with max turbo
                       frequency as the limit.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_region_summary.py`:

  Generates per-region summaries of the report data.

#### `gen_frequency_map.py`:

  Calculates the best-fit frequencies for each region in a frequency
  for a given maximum performance degradation (runtime increase).

#### `gen_plot_runtime_energy.py`:

  Creates a plot of the runtime and energy of a region of interest
  versus the fixed frequency from a frequency sweep.
