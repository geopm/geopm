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

## Other Experiment Script(s)

#### `neural_net_sweep.py`

  Runs a frequency sweep and gathers the report and trace signals required to generate
  the region characterization neural net. The signals gathered will include GPU signals
  if GPUs are detected on the system. Otherwise, only CPU signals will be gathered at
  the package scope. See the `gpu_frequency_sweep` experiment for extra details.

## Scripts to Produce Neural Nets and Frequency Recommendation Maps

#### `gen_hdf_from_fsweep.py`:

  Generates HDF files used to generate the neural net and frequency recommendation json files.
  This takes report and trace files from frequency sweep experiments as inputs, checks for
  required signals on CPU and GPU domains, annotates data with microbenchmark and node names,
  and outputs the hdfs. Two files are generated: The stats file contains per-region report
  information used to determine frequency recommendation for each region class. The trace file
  contains trace data that is used to create a neural net that determines region class 
  probabilities during a workload execution.

  This script requires the following positional inputs:

  - `output`: Prefix for output files [output]_stats.h5 and [output]_traces.h5
  - `frequency_sweep_dirs`: Directories containing reports and traces from frequency sweeps

#### `gen_neural_net.py`:

   Generates neural net json file(s) for CPU and/or GPU. This takes in the trace h5 file generated 
   from gen_hdf_from_fsweep.py which is annotated with region classes. Required signals are 
   specified within this script, per-domain. The script checks for the complete list of signals
   and generates the corresponding neural nets.

   This script can take in the following inputs:

   - `output`: Prefix of the output json file(s).
   - `description`: Description of the neural net
   - `ignore` : A comma-separated list of region hashes to ignore
   - `data` : Data files to train on. This can take in multiple trace HDF files.

   Note that this script depends upon pytorch. Pytorch can be installed using: `pip install pytorch`

### `gen_region_parameters.py`:

   Generates region-class frequency recommendation map. This takes in the stats h5 file
   generated from gen_hdf_from_fsweep.py, which is annotated with region classes and
   contains runtime and per-domain energy information. This is used to generate a
   runtime-frequency relationship and determine minimum energy point within an allowable
   perf degradation. For each region class, a list is generated which contains the 
   desired frequency for a region class for various perf-energy-bias values, from
   most perf-sensitive to most energy-sensitive.

   This script takes in the following input:

   - `data-file`: The stats HDF generated from gen_hdf_from_fsweep.py.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_phi_sweep_graph.py`:

  Generates a graph that shows performance degradation and energy savings for different
  values of perf-energy-bias.
