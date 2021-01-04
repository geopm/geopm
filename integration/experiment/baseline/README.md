BASELINE EXPERIMENTS
--------------------

The scripts in this directory are used to run applications with the
monitor agent and without the GEOPM controller to collect data on
baseline performance using different core counts.  For applications
that scale to use all cores of a processor, performance may be impacted
if either cores are reserved for the GEOPM controller, or if the GEOPM
controller is run on one of the application cores.


## Base Experiment Module

#### `baseline.py`:

  Contains helper functions for launching.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_summary.py`:

  Prints a table of the total runtime and figure of merit, averaged
  across all trials.