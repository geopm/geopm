Feed-Forward Net-Based Experiments
----------------------------------

The scripts in this directory are used to run and analyze application
runs that use the ffnet agent with a specified path to neural net and
frequency recommendation maps, optionally with a specified perf energy bias.

## Base Experiment Module

#### `ffnet.py`:

  Contains the helper function for launching an ffnet experiment.

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

## Additional Experiment Module
#### `neural_net_sweep.py`:

  Contains the helper function for launching a frequency sweep experiment, the data from
  which can be used to generate neural net and region parameter json files. This function
  executes a CPU/GPU/uncore frequency sweep while gathering telemetry that is useful to 
  differentiate regions of interest.

  In addition to command line arguments common to all runscripts, this script also accepts
  options used by the `gpu_frequency_sweep experiment`:

  **CPU Frequency Settings**

  - `--max-frequency`: the maximum CPU frequency setting for the sweep.
                       By default, it uses the sticker for the compute
                       nodes, and executes an additional run with max
                       turbo frequency as the limit.  Within each
                       trial, the experiment script will start at the
                       highest frequency and run at each limit until
                       it reaches the minimum.

  - `--min-frequency`: the minimum CPU frequency setting for the sweep.

  - `--step-frequency`: the step size in hertz between CPU frequency
                        settings for the sweep.
 
  - `--run-max-turbo`: executes additional runs with max turbo
                       frequency as the limit for the core frequency.

  **CPU Uncore Frequency Settings**
 
  - `--max-uncore-frequency`: the maximum uncore frequency setting for
                              the sweep.  By default, it uses 2.7 GHz.

  - `--min-uncore-frequency`: the minimum uncore frequency setting for
                              the sweep.  By default, it uses 1.2 GHz.

  - `--step-uncore-frequency`: the step size in hertz between uncore
                               frequency settings for the sweep.

  **GPU Frequency Settings**

  - `--max-gpu-frequency`: the maximum GPU frequency setting for the sweep.
                           By default, it uses the maximum frequency for the
                           GPUs on the node. 

  - `--min-gpu-frequency`: the minimum GPU frequency setting for the sweep.
                           By default, it uses the minimum frequency for the
                           GPUs on the node.

  - `--step-gpu-frequency`: the step size in hertz between settings for the sweep.
                            By default, it uses the minimum supported step size
                            of GPUs on the node.

  Note that for any frequency domain, if the values above are not specified or if
  the min and max values are equal, a frequency sweep will not be executed across
  that domain.

## Supporting Files

#### `gen_hdf_from_fsweep.py`:

  Generates HDF files used to generate the neural net and frequency recommendation json files.
  This takes report and trace files from frequency sweep experiments as inputs, checks for
  required signals on CPU and GPU domains, annotates data with microbenchmark and node names,
  and outputs the HDFs. Two files are generated: The stats file contains per-region report
  information used to determine frequency recommendation for each region class. The trace file
  contains trace data that is used to create a neural net that determines region class 
  probabilities during a workload execution.

  This script requires the following positional inputs:

  - `output`: Prefix for output files `[output]_stats.h5` and `[output]_traces.h5`
  - `frequency_sweep_dirs`: Directories containing reports and traces from frequency sweeps
    
  E.g.:

   ```
   ./gen_hdf_from_fsweep.py example_output /path/to/fsweep
   ```
   This will generate `example_output_stats.h5` and `example_output_traces.h5`

#### `gen_neural_net.py`:

   Generates neural net json file(s) for CPU and/or GPU. This takes in the trace h5 file generated 
   from `gen_hdf_from_fsweep.py` which is annotated with region classes. Required signals are 
   specified within this script, per-domain. The script checks for the complete list of signals
   and generates the corresponding neural nets.

   This script can take in the following inputs:

   - `output`: Prefix of the output json file(s).
   - `description`: Description of the neural net
   - `ignore` : A comma-separated list of region hashes to ignore
   - `data` : Data files to train on. This can take in multiple trace HDF files.

   Note that this script depends upon pytorch. Pytorch can be installed using: `pip install pytorch`
  
   E.g.:

   ```
   ./gen_neural_net.py --output nnet --description "A neural net" --data example_output_traces.h5
   ```
   This will generate `nnet_cpu.json` and/or `nnet_gpu.json`

#### `gen_region_parameters.py`:

   Generates region-class frequency recommendation map. This takes in the stats h5 file
   generated from `gen_hdf_from_fsweep.py`, which is annotated with region classes and
   contains runtime and per-domain energy information. This is used to generate a
   runtime-frequency relationship and determine minimum energy point within an allowable
   perf degradation. For each region class, a list is generated which contains the 
   desired frequency for a region class for various perf-energy-bias values, from
   most perf-sensitive to most energy-sensitive.

   This script takes in the following input:

   - `output`: Prefix of the output json file(s).
   - `data-file`: The stats HDF generated from `gen_hdf_from_fsweep.py`.
   
   E.g.:

   ```
    ./gen_region_parameters.py --output fmap --data-file example_output_stats.h5
   ```

   This will generate `fmap_cpu.json` and/or `fmap_gpu.json`

#### `gen_sweep_to_ffnet.py`:

   Executes scripts referenced above to: Generate HDFs, generate neural nets, and generate
   region-class frequency recommendation maps.

   This script takes in the following input:

   - `output`: prefix of the output HDF and json file(s)
   - `description`: Description of the neural net
   - `ignore` : A comma-separated list of region hashes to ignore
   - `frequency_sweep_dirs`: Directories containing reports and traces from frequency sweeps

   This will generate all HDFs and json files referenced above.
  
   E.g. 
   ```
   ./gen_sweep_to_ffnet.py --output test --description "Test" --frequency_sweep_dirs /path/to/fsweep
   ```
   This will generate the following files:
   - `test_stats.h5`
   - `test_traces.h5`
   - `test_nn_cpu.json`  and/or `test_nn_gpu.json`
   - `test_fmap_gpu.json` and/or `test_fmap_gpu.json`
   

## Scripts to Produce Neural Nets and Frequency Recommendation Maps

The end-to-end process for utilizing this agent to improve energy efficiency can be conducted
using the following steps.

1. Conduct a frequency sweep on a set of microbenchmarks, gathering the required signals.
   This can be done using `neural_net_sweep.py`. Microbenchmarks that have been shown to
   generate useful results include Arithmetic Intensity Benchmark and geopmbench on CPU,
   and the PARRES suite (DGEMM and STREAM) on GPU.

2. Generate neural net json files and region frequency recommendation map files using
   `gen_sweep_to_ffnet.py`. E.g.:

   ```
   ./gen_sweep_to_ffnet.py --output test --description "Test" --frequency_sweep_dirs /path/to/fsweep
   ```

   Resulting json files will output in the directory in which the script was executed
   and filenames will be prepended with the user-provided prefix. E.g. output files:

   - CPU Neural Net: `test_nn_cpu.json`
   - CPU Region Frequency Recommendation Map: `test_fmap_cpu.json`
   - GPU Neural Net: `test_nn_gpu.json`
   - GPU Region Frequency Recommendation Map: `test_fmap_gpu.json`


3. Set environment variables to point to the generated json files. E.g.:

```
    export GEOPM_CPU_NN_PATH=${GEOPM_SOURCE}/integration/experiment/ffnet/test_nn_cpu.json
    export GEOPM_GPU_NN_PATH=${GEOPM_SOURCE}/integration/experiment/ffnet/test_nn_gpu.json

    export GEOPM_CPU_FMAP_PATH=${GEOPM_SOURCE}/integration/experiment/ffnet/test_fmap_cpu.json
    export GEOPM_GPU_FMAP_PATH=${GEOPM_SOURCE}/integration/experiment/ffnet/test_fmap_gpu.json
``` 

4. Run your workload with the ffnet agent.

## Analysis Scripts to Produce Summary Tables and Visualizations

#### `gen_phi_sweep_graph.py`:

  Generates a graph that shows performance degradation and energy savings for different
  values of perf-energy-bias.
