FREQUENCY SWEEP-BASED EXPERIMENTS
---------------------------------

The scripts in this directory are used to run and analyze application
runs that use the ffnet agent with a specified path to neural net and
frequency recommendation maps, optionally with a specified perf energy bias.

## Base Experiment Module

#### `ffnet.py`:

  Contains the helper function for launching an ffnet experiment and
  common command line arguments.

  In addition to command line arguments common to all run scripts,
  ffnet agent runs also accept the following options:

  - `perf-energy-bias`: A bias [0-1] that indicates the amount of performance
                        degradation that is acceptable in order to achieve an 
                        improvement in energy efficiency. A value of 0 indicates
                        that performance degradation is not tolerated. A value
                        of 1 indicates that energy efficiency is of utmost
                        importance. Note that no values indicate a guaranteed
                        amount of performance degradation or energy savings.
  - `cpu-nn-path`: A path to a neural network for guiding CPU frequency decisions.
                   Frequency steering will occur at the package scope. If the path
                   does not exist, an error will be thrown. At least one of 
                   cpu-nn-path and gpu-nn-path must be specified or an error will be
                   thrown.
  - `gpu-nn-path`: A path to a neural network json for guiding GPU frequency decisions.
                   Frequency steering will occur at the gpu scope. If the path
                   does not exist, an error will be thrown. At least one of 
                   cpu-nn-path and gpu-nn-path must be specified or an error will be
                   thrown.
  - `cpu-fmap-path`: A path to the CPU frequency recommendation map json. This file
                     must contain region keys specified in the file at cpu-nn-path
                     and must map each of these regions to an array of frequency 
                     decisions for different values of perf-energy-bias. If cpu-nn-path
                     is specified but cpu-fmap-path is not (or its path is invalid),
                     an error will be thrown.
  - `gpu-fmap-path`: A path to the GPU frequency recommendation map json. This file
                     must contain region keys specified in the file at gpu-nn-path
                     and must map each of these regions to an array of frequency 
                     decisions for different values of perf-energy-bias. If gpu-nn-path
                     is specified but gpu-fmap-path is not (or its path is invalid),
                     an error will be thrown.


## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_phi_sweep_graph.py`:

  Generates a graph that shows performance degradation and energy savings for different
  values of perf-energy-bias.
