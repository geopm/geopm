GPU Frequency Sweep-Based Experiments
-------------------------------------

The scripts in this directory are used to run and analyze application
runs that use a range of fixed frequency settings for GPUs.

## Base Experiment Module

#### `gpu_frequency_sweep.py`:

  Contains the helper function for launching a GPU frequency sweep and
  common command line arguments.

  In addition to command line arguments common to all run scripts,
  the GPU frequency sweep script also accepts options listed below.
  Within each trial, the experiment script will start at the highest 
  frequency and run at each frequency until it reaches the minimum.

  - `--max-gpu-frequency`: the maximum GPU frequency setting for the sweep.
                           By default, it uses the maximum frequency for the
                           GPUs on the node. 

  - `--min-gpu-frequency`: the minimum GPU frequency setting for the sweep.
  -                        By default, it uses the minimum frequency for the
  -                        GPUs on the node.

  - `--step-gpu-frequency`: the step size in hertz between settings for the sweep.
  -                         By default, it uses the minimum supported step size
  -                         of GPUs on the node.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_gpu_activity_constconfig_recommendation.py`:

  Calculates the GPU energy efficient frequency and provides a policy
  recommendation for the `gpu_activity` agent.
