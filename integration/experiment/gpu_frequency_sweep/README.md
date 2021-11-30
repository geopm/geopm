GPU FREQUENCY SWEEP-BASED EXPERIMENTS
----------------------------------------

The scripts in this directory are used to run and analyze application
runs that use a range of settings for the accelerator, the core and
the uncore frequency.

## Base Experiment Module

#### `gpu_frequency_sweep.py`:

  Contains the helper function for launching a frequency sweep and
  common command line arguments.

  In addition to command line arguments common to all run scripts,
  frequency sweep runs also accept the following options:

  - `--max-frequency`: the maximum core frequency setting for the
                       sweep.  By default, it uses the sticker
                       for the compute nodes.  Within each trial,
                       the experiment script will start at the
                       highest frequency and run at each limit
                       until it reaches the minimum.

  - `--min-frequency`: the minimum core frequency setting for the sweep.

  - `--step-frequency`: the step size in hertz between core
                        frequency settings for the sweep.

  - `--run-max-turbo`: executes additional runs with max turbo
                       frequency as the limit for the core frequency.

  - `--max-uncore-frequency`: the maximum uncore frequency setting for
                              the sweep.  By default, it uses 2.7 GHz.

  - `--min-uncore-frequency`: the minimum uncore frequency setting for
                              the sweep.  By default, it uses 1.2 GHz.

  - `--step-uncore-frequency`: the step size in hertz between uncore
                               frequency settings for the sweep.			       

  - `--max-gpu-frequency`: the maximum gpu frequency setting for
                           the sweep.  By default, it uses max allowed.

  - `--min-gpu-frequency`: the minimum gpu frequency setting for
                           the sweep.  By default, it uses min allowed.

  - `--step-gpu-frequency`: the step size in hertz between gpu
                            frequency settings for the sweep.
			       

## Analysis Scripts to Produce Summary Tables and Visualizations

